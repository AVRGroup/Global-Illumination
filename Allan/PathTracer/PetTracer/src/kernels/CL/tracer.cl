#include <path.cl>
#include <primitives.cl>
#include <random.cl>
#include <bvh.cl>

void IntersectScene( SceneData const* scenedata, Ray* r, Intersection* isect )
{
	isect->shapeID = -1;
	isect->primID = -1;
	isect->uvwt = (float4)( 0.0f, 0.0f, 0.0f, 1e20f );
	r->o.w = 1e20f;

	/*for ( int i = 0; i < sphereCount; i++ )
	{
		float hitDistance = IntersectSphere( &scene[i], r );

		if ( hitDistance != 0.0f && hitDistance < isect->uvwt.w )
		{
			isect->shapeID = i;
			isect->uvwt.w = hitDistance;
		}
	}*/

	IntersectSceneClosest( scenedata, r, isect );

	/*BBox meshBB;
	meshBB.pmin = ( float4 )( -0.10f, -0.10f, -0.10f, 0.0f );
	meshBB.pmax = ( float4 )( 0.10f, 0.10f, 0.10f, 0.0f );
	const float3 invDir = makeFloat3( 1.0f, 1.0f, 1.0f ) / ( r->d.xyz );
	if ( IntersectBox( r, invDir, meshBB, 1e20f ) )
	{
		isect->uvwt.x = 1.0f;
	}*/
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
				__global float4* vertices,		//0
				__global int4*	 faces,			//1
				__global BBox*	 nodes,			//2
				// Rays input
				__global Ray* rays,				//3
				unsigned int numRays,			//4
				// Ray hit output
				__global Intersection* hits,	//5
				// Debug output
				__global float3* output,		//6
				unsigned int width,				//7
				unsigned int height				//8
				)
{
	int globalID = get_global_id( 0 );
	int localID = get_local_id( 0 );

	SceneData scenedata = { nodes, vertices, faces };

	// check for work
	if ( globalID < numRays )
	{
		Ray r = rays[globalID];

		

		Intersection isect;
		IntersectScene( &scenedata, &r, &isect );

		hits[globalID] = isect;

		// Debug
		int x_coord = globalID % width;
		int y_coord = globalID / width;

		float fx = ( ( float ) (x_coord + 0.0001f) / ( float ) width ) * 2.0f - 1.0f;
		float fy = ( ( float ) (y_coord + 0.0001f) / ( float ) height ) * 2.0f - 1.0f;

		union Colour { float c; uchar4 components; } fcolour;

		float3 heatColor = isect.uvwt.w / ( isect.uvwt.w + 1.0f );
		heatColor.z = 0.125f;

		fcolour.components = ( uchar4 )
				( ( unsigned char )( ( heatColor.x ) * 255 ),
				( unsigned char )( ( heatColor.y ) * 255 ),
				( unsigned char )( ( heatColor.z ) * 255 ),
				1 );

		/*fcolour.components = ( uchar4 )
			( ( unsigned char )( ( fx / ( fx + 1 ) ) * 255 ),
			( unsigned char )( ( fy / ( fy + 1 ) ) * 255 ),
				( unsigned char )( ( isect.uvwt.w / ( isect.uvwt.w + 1 ) ) * 255 ),
				1 );*/

		output[globalID] = ( float3 )( fx, fy, fcolour.c );

	}
}
