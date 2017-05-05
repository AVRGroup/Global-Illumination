#include <common.cl>
#include <ray.cl>
#include <path.cl>
#include <primitives.cl>
#include <random.cl>
#include <bvh.cl>
#include <camera.cl>
#include <scene.cl>
#include <material.cl>
#include <bxdf.cl>

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

		
		if ( Ray_IsActive( &r ) )
		{
			Intersection isect;
			IntersectScene( &scenedata, &r, &isect );

			hits[globalID] = isect;
		}
	}
}

__kernel void EvaluateVolume(
	// Rays
	__global	Ray			const*	rays,
	// Pixel indices
	__global	int			const*	pixelIndices,
	// Number of rays
	__global	int			const*	numRays,
	// Volumes
				int					volumes,
	// Textures
				int					textures,
				int					textureData,
	// RNG seed
				uint				rngseed,
	// Sampler state
	__global	uint			 *	random,
				int					randomUtil,
	// Current pass
				int					bounce,
	// Current frame
				int					frame,
	// Intersection data
	__global	Intersection	 *	isects,
	// Current paths
	__global	Path			 *	paths,
	// Output
	__global	float3			 *	output
)
{
	int globalID = get_global_id( 0 );

	if(globalID < *numRays)
	{
		int pixelID = pixelIndices[globalID];

		__global Path* path = paths + pixelID;

		// Path can be dead
		if ( !Path_IsAlive( path ) )
			return;

		int volID = Path_GetVolumeIndex( path );

		// Check if we are inside some volume
		if( volID != 1 )
		{
		
		}
	}

}

__kernel void FilterPathStream(
	// Intersections
	__global Intersection	const*	isects,
	// Number of compacted indices
	__global int			const*	numItens,
	// Pixel indices
	__global int			const*	pixelIndices,
	// Paths
	__global Path				 *	paths,
	// Predicate
	__global int				 *	predicate
)
{
	int globalID = get_global_id( 0 );

	if(globalID < *numItens)
	{
		int pixelID = pixelIndices[globalID];

		__global Path* path = paths + pixelID;

		if (Path_IsAlive(path))
		{
			bool kill = ( length( Path_GetThroughput( path ) ) < 0.001f ); // See here later

			if(!kill)
			{
				predicate[globalID] = isects[globalID].primID >= 0 ? 1 : 0;
			}
			else
			{
				Path_Kill( path );
				predicate[globalID] = 0;
			}
		}
		else
		{
			predicate[globalID] = 0;
		}
	}
}

__kernel void RestorePixelIndices(
	// Compacted indices
	__global int	const * compactedIndices,
	// Number of compacted indices
	__global int	const*	numItens,
	// Previous pixel indices
	__global int	const*	prevIndices,
	// New pixel indices
	__global int		 *	newIndices
)
{
	int globalID = get_global_id( 0 );

	if(globalID < *numItens)
	{
		newIndices[globalID] = prevIndices[compactedIndices[globalID]];
	}

}

__kernel void ShadeMiss(
	// Rays
	__global Ray			const*	rays,
	// Intersections
	__global Intersection	const*	isects,
	// Pixel indices
	__global int			const*	pixelIndices,
	// Number of rays
			 int					numRays,
	// Textures
			 int					textures,
			 int					textureData,
	// Enviroment map ID
			 int					envmapID,
	// Path
	__global Path			const*	paths,
	// Volumes
			 int					volumes,
	// Output volume
	__global float4*				output
)
{
	int globalID = get_global_id( 0 );

	if(globalID < numRays)
	{
		int pixelID = pixelIndices[globalID];

		// In case of a miss
		if( isects[globalID].shapeID < 0 )
		{
			int volID = paths[pixelID].volume;

			// If the ray didnt pass a volume
			if(volID == -1)
			{
				//Apply the enviroment map
				output[pixelID].xyzw += makeFloat4( 0.1f, 0.125f, 0.137f, 1.0f );
			}
			else
			{
				// Apply volume emission
			}

		}
	}
}

__kernel void ShadeVolume(
	// Rays
	__global Ray			const*	rays,
	// Intersection data
	__global Intersection	const*	isects,
	// Hit indices
	__global int			const*	hitIndices,
	// Pixel indices
	__global int			const*	pixelIndices,
	// Number of rays
	__global int			const*	numRays,
	// Vertices
	__global float3			const*	vertices,
	// Normals
	__global float3			const*	normals,
	// UVs
	__global float2			const*	uvs,
	// Indices
	__global int			const*	indices,
	// Shapes
			 int					shapes,
	// Material IDs
			 int					materialIDS,
	// Materials
	__global Material		const*	materials,
	// Textures
			 int					textures,
			 int					textureData,
	// Enviroment map id
			 int					envmapidx,
	// Envmap multiplier
			 float					envmapmul,
	// Emissives
			 int					lights,
	// Number of emissive objects
			 int					num_lights,
	// RNG seed
			 uint					rngSeed,
	// Sampler state
	__global uint				 *	random,
	// Sobol matrices
			 uint					sobolmat,
	// Current bounce
			 int					bounce,
	// Current frame
			 int					frame,
	// Volume data
			 int					volumes,
	// Shadow rays
	__global Ray				 *	shadowRays,
	// Light samples
			 int					lightSamples,
	// Path throughput
	__global Path				 *	paths,
	// Indirect rays (next path segment)
	__global Ray				 *	indirectrays,
	// Radiance
	__global float3				 *	output
)
{

}

__kernel void ShadeSurface(
	// Rays
	__global Ray			const*  rays,
	// Intersections
	__global Intersection	const*  isects,
	// Hit indices
	__global int			const*	hitIndices,
	// Pixel indices
	__global int			const*	pixelIndices,
	// Number of rays
	__global int			const*	numRays,
	// Geometry vertices
	__global float3			const*	vertices,
	// Geometry normals
	__global float3			const*	normals,
	// Geometry UVs
	__global float2			const*	uvs,
	// Indices
	__global int4			const*	indices,
	// Shapes
			 int					shapes,
	// Material Indices
			 int					materialIds,
	// Materials
	__global Material		const*  materials,
	// Texture
			 int					textures,
			 int					textureData,
	// Enviroment map texture ID
			 int					envmapID,
	// Enviroment map multiplier
			 float					envmapMul,
	// Emissives
			 int					lights,
	// Number of emissive objects
			 int					numLights,
	// RNG seed
	         uint					rngSeed,
	// Sampler states
	__global uint 				 *	random,
	// Sobol matices
			 int					sobolMat,
	// Current Bounce
	         int 					bounce,
	// Current frame
			 int 					frame,
	// Volume data
			 int					volume,
	// Shadow
			 int					shadowRays,
	// Light samples
			 int					lightSamples,
	// Path data
	__global Path 				 *	paths,
	// Indirect rays
	__global Ray				 *	indirectRays,
	// Radiance accum buffer
	__global float3 			 *	output

)
{
	int globalID = get_global_id(0);

	Scene scene =
	{
		vertices,
		normals,
		uvs,
		indices,
		materials
	};

	if(globalID < *numRays)
	{
		int			 hitID	 = hitIndices[globalID];
		int			 pixelID = pixelIndices[globalID];
		Intersection isect   = isects[hitID];

		__global Path* path  = paths + pixelID;

		// Exit if path has been scattered
		if( Path_IsScattered(path) )
		{
			return;
		}


		// Incoming ray direction
		float3 wi = -normalize( rays[hitID].d.xyz );

		Sampler sampler;
#if SAMPLER == RANDOM
		uint scramble = pixelID * rngSeed;
		Sampler_Init( &sampler, scramble );
#elif SAMPLER == CMJ
		uint rnd = random[pixelID];
		uint scramble = rnd * 0x1fe3434f * ( ( frame + 331 * rnd ) / ( CMJ_DIM * CMJ_DIM ) );
		Sampler_Init( &sampler, frame % ( CMJ_DIM * CMJ_DIM ), SAMPLE_DIM_SURFACE_OFFSET + bounce * SAMPLE_DIMS_PER_BOUNCE, scramble );
#endif

		// Fill surface data
		DifferentialGeometry diffgeo;
		DifferentialGeometry_Fill( &scene, &isect, &diffgeo );

		// Check if its a backface
		float ngdotwi = dot( diffgeo.ng, wi );
		bool backfacing = ngdotwi < 0.0f;

		float3 throughput = Path_GetThroughput( path );

		// If the material iss emissive, add the contribution and terminate
		if( NON_ZERO(diffgeo.mat.emissive) )
		{
			if ( !backfacing )
			{
				float weight = 1.f;

				if ( bounce > 0 && !Path_IsSpecular( path ) )
				{
					float2 extra = Ray_GetExtra( &rays[hitID] );
					float ld = isect.uvwt.w;
					float denom = fabs( dot( diffgeo.n, wi ) ) * diffgeo.area;
					// TODO: num_lights should be num_emissies instead, presence of analytical lights breaks this code
					float bxdflightpdf = denom > 0.f ? ( ld * ld / denom / 2) : 0.f;
					weight = BalanceHeuristic( 1, extra.x, 1, bxdflightpdf );
				}

				output[pixelID] += throughput * 100 * diffgeo.mat.emissive.xyz * weight;
			}
			Path_Kill( path );
			Ray_SetInactive( indirectRays + globalID );
			
			return;
		}

		// Invert normal if hit the face inside
		float s = 1.0f;
		if( backfacing )
		{
			diffgeo.n = -diffgeo.n;
			diffgeo.dpdu = -diffgeo.dpdu;
			diffgeo.dpdv = -diffgeo.dpdv;
			s = -s;
		}

		//float ndotwi = fabs( dot( diffgeo.n, wi ) );

		float2 bxdfSampler = Sampler_Sample2D( &sampler );
		float3 bxdfwo;
		float bxdfpdf;
		float3 bxdf = Bxdf_Sample( &diffgeo, wi, bxdfSampler, &bxdfwo, &bxdfpdf );


		// Apply Russian roulette
		float q = max( min( 0.5f,
			// Luminance
			0.2126f * throughput.x + 0.7152f * throughput.y + 0.0722f * throughput.z ), 0.01f );
		// Only apply if is 3+ bounces
		bool rrApply = bounce > 3;
		bool rrStop = Sampler_Sample1D( &sampler ) > q && rrApply;

		if(rrApply)
		{
			Path_MulThroughput( path, native_recip(q) );
		}


		bxdfwo = normalize( bxdfwo );
		float3 t = bxdf * fabs( dot( diffgeo.n, bxdfwo ) );
		/*if ( true )
		{
			float2 sample = Sampler_Sample2D( &sampler );
			float rand1 = 2.0f * PI * sample.x;
			float rand2 = sample.y;
			float rand2s = sqrt( rand2 );

			newDir = normalize( diffgeo.dpdu * cos( rand1 )*rand2s + diffgeo.dpdv*sin( rand1 )*rand2s + diffgeo.n*sqrt( 1.0f - rand2 ) );
		}*/

		// Only continue if we have non-zero throughput & pdf
		if( NON_ZERO(t) && bxdfpdf > 0.0f && !rrStop )
		{
			Path_MulThroughput( path, t / bxdfpdf /*diffgeo.mat.albedo.xyz * dot( newDir, diffgeo.n )*/ );

			float3 indirectRayOrigin = diffgeo.p + 0.001f * s *diffgeo.ng;

			// Generate Ray
			Ray_Init( indirectRays + globalID, indirectRayOrigin, bxdfwo, 1000000.f, 0.0f, 0xFFFFFFFF );
			Ray_SetExtra( indirectRays + globalID, makeFloat2( bxdfpdf, 0.0f ) );
		}
		else
		{
			// Otherwise kill the path
			Path_Kill( path );
			Ray_SetInactive( indirectRays + globalID );
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
