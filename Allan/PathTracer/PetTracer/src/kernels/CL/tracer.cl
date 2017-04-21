#include <common.cl>
#include <ray.cl>
#include <path.cl>
#include <primitives.cl>
#include <random.cl>
#include <bvh.cl>
#include <camera.cl>
#include <scene.cl>
#include <material.cl>

void IntersectScene( SceneData const* scenedata, Ray* r, Intersection* isect )
{
	isect->shapeID = -1;
	isect->primID = -1;
	isect->uvwt = (float4)( 0.0f, 0.0f, 0.0f, 1e20f );
	r->o.w = 1e20f;

	IntersectSceneClosest( scenedata, r, isect );
}

float3 getHeatMapColor( float value )
{
	const float3 color[4] = { makeFloat3( 0,0,1 ), makeFloat3( 0,1,0 ), makeFloat3( 1,1,0 ), makeFloat3( 1,0,0 ) };
	// A static array of 4 colors:  (blue,   green,  yellow,  red) using {r,g,b} for each.

	int idx1;        // |-- Our desired color will be between these two indexes in "color".
	int idx2;        // |
	float fractBetween = 0;  // Fraction between "idx1" and "idx2" where our value is.

	if ( value <= 0 ) { idx1 = idx2 = 0; }    // accounts for an input <=0
	else if ( value >= 1 ) { idx1 = idx2 = 4 - 1; }    // accounts for an input >=0
	else
	{
		value = value * ( 4 - 1 );        // Will multiply value by 3.
		idx1  = floor( value );                  // Our desired color will be after this index.
		idx2  = idx1 + 1;                        // ... and before this index (inclusive).
		fractBetween = value - (float)( idx1 );    // Distance between the two indexes (0-1).
	}

	return ( color[idx2] - color[idx1] )*fractBetween + color[idx1];
}


__attribute__( ( reqd_work_group_size( 64, 1, 1 ) ) )
__kernel void IntersectClosest(
				// Scene description
				__global float3* vertices,		//0
				__global int4*	 faces,			//1
				__global BBox*	 nodes,			//2
				// Rays input
				__global Ray*	 rays,			//3
				__global int*	 numRays,		//4
				// Ray hit output
				__global Intersection* hits,	//5
				// Debug output
				__global float3* output,		//6
				unsigned int width,				//7
				unsigned int height			    //8
				)
{
	int globalID = get_global_id( 0 );
	int localID = get_local_id( 0 );

	SceneData scenedata = { nodes, vertices, faces };

	// check for work
	if ( globalID < *numRays )
	{
		Ray r = rays[globalID];

		

		Intersection isect;
		IntersectScene( &scenedata, &r, &isect );

		hits[globalID] = isect;

	}
}

__kernel void ShadeSurface(
	// Rays
	__global Ray          const*    rays,
	// Output Rays
	__global Ray   		  	   *	outRays,
	// Intersections
	__global Intersection const*    isects,
	// Number of rays
	__global int          const*    numRays,
	// Number of output rays
	__global int 		       *    nOutRays,
	// Geometry vertices
	__global float3       const*    vertices,
	// Geometry normals
	__global float3       const*    normals,
	// Geometry UVs
	__global float4       const*    uvs,
	// Indices
	__global int4         const*    indices,
	// Surfaces
	// TODO
	// Materials
	__global Material	  const*    materials,
	// RNG seed
	         uint					rngSeed,
	// Sampler states
	__global Rng 			   * 	random,
	// Current Bounce
	         int 					bounce,
	// Current frame
	//		 int 					frame,
	// Path data
	__global Path 			   * 	paths,
	// Radiance accum buffer
	__global float3 		   * 	output

)
{
	int globalID = get_global_id(0);

	Scene scene = {
		vertices,
		normals,
		uvs,
		indices
	};

	if(globalID < *numRays)
	{
		__global Ray  		  const* r     = rays   + globalID;
		__global Intersection const* isect = isects + globalID;
				 int         		 idx   = Ray_GetPixel(r);
		__global Path 		  const* path  = paths  + idx;
		//__global float3			   * accum = output + idx;



		float3 contrib;
		if ( isect->shapeID != -1 && length(Path_GetThroughput(path)) > EPSILON )
		{

			const int flt = 0;
			__global Material *mat = materials + isect->shapeID;
			const float3 color    = mat->albedo.xyz;
			const float3 emissive = mat->emissive.xyz;
			/*const float3 color    = ( ( isect->shapeID % 2 ) == 0 ) ? makeFloat3( 0.75f, 0.75f, 0.75f ) : makeFloat3( 00.0f, 00.0f, 00.0f );
			const float3 emissive = ( ( isect->shapeID % 2 ) == 0 ) ? makeFloat3( 0.00f, 0.00f, 0.00f ) : makeFloat3( 2.0f, 2.0f, 2.0f );*/

			float3 hitPoint = r->o.xyz + isect->uvwt.w * r->d.xyz;

			float3 normal = isect->uvwt.xyz;
			float3 normalFacing = dot( normal, r->d.xyz ) < 0.0f ? normal : normal * ( -1.0f );

			float3 newDir;

			if ( flt == 0 )
			{
				Rng seed0 = random[idx];
				seed0.val += rngSeed + ( globalID << 2 ) * 157 + 13;

				float rand1 = 2.0f * PI *  RandFloat( &seed0 );
				float rand2 = RandFloat( &seed0 );
				float rand2s = sqrt( rand2 );

				float3 w = normalFacing;
				float3 axis = fabs( w.x ) > 0.1f ? ( float3 )( 0.0f, 1.0f, 0.0f ) : ( float3 )( 1.0f, 0.0f, 0.0f );
				float3 u = normalize( cross( axis, w ) );
				float3 v = cross( w, u );

				newDir = normalize( u * cos( rand1 )*rand2s + v*sin( rand1 )*rand2s + w*sqrt( 1.0f - rand2 ) );
				
				random[idx] = seed0;
			}
			else
			{
				newDir = normalize( r->d.xyz - normalFacing * 2.0f * dot( normalFacing, r->d.xyz ) );
			}


			Ray ray;
			ray.o = ( float4 )( hitPoint + normalFacing * EPSILON, 0.0f );
			ray.d = ( float4 )( newDir, r->d.w );
			ray.extra.x = idx;

			int _idx = atomic_inc( nOutRays );
			outRays[_idx] = ray;

			Path_AddContribution( path, output, idx, emissive );
			//contrib = emissive * Path_GetThroughput( path );			
			Path_MulThroughput( path, color * dot( newDir, normalFacing ) );

		}
		else
		{
			contrib = makeFloat3( 0.0f, 0.0f, 0.0f );
			Path_MulThroughput( path, contrib );
			//Path_AddContribution( path, accum, idx, makeFloat3( 0.3f, 0.64f, 0.95f ) );
		}
	}

}


__kernel void Accumulate(
		         int	 numPixel,	//0
		__global float3* accum,		//1
		__global float3* output,    //2
		unsigned int     width,		//3
		unsigned int     height,	//4
	    unsigned int     frame      //5
	)
{
	int globalID = get_global_id( 0 );

	// check for work
	if ( globalID < numPixel )
	{
		int x_coord = globalID % width;
		int y_coord = globalID / width;

		float fx = ( ( float ) ( x_coord + 0.0001f ) / ( float ) width ) * 2.0f - 1.0f;
		float fy = ( ( float ) ( y_coord + 0.0001f ) / ( float ) height ) * 2.0f - 1.0f;

		union Colour { float c; uchar4 components; } fcolour;

		float3 rawColor =  accum[globalID] * native_recip( frame );
		float3 finalColor = rawColor / ( rawColor + 1.0f );

		fcolour.components = ( uchar4 )
			    ( ( unsigned char )( ( finalColor.x ) * 255 ),
			      ( unsigned char )( ( finalColor.y ) * 255 ),
				  ( unsigned char )( ( finalColor.z ) * 255 ),
				  1 );

		output[globalID] = ( float3 )( fx, fy, fcolour.c );

	}
}
