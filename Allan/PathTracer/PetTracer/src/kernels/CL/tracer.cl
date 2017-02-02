#include <path.cl>
#include <primitives.cl>
#include <random.cl>


void IntersectSceneClosest( __global Sphere* scene, const int sphereCount, Ray* r, Intersection* isect )
{
	isect->shapeID = -1;
	isect->primID = -1;
	isect->uvwt = (float4)( 0.0f, 0.0f, 0.0f, 1e20f );

	for ( int i = 0; i < sphereCount; i++ )
	{
		float hitDistance = IntersectSphere( &scene[i], r );

		if ( hitDistance != 0.0f && hitDistance < isect->uvwt.w )
		{
			isect->shapeID = i;
			isect->uvwt.w = hitDistance;
		}
	}
}


__kernel
void IntersectClosest(
				// Scene description
				__global Sphere* spheres,
				const int sphereCount,
				// Rays input
				__global Ray* rays,
				unsigned int numRays,
				// Ray hit output
				__global Intersection* hits,
				// Debug output
				__global float3* output,
				unsigned int width,
				unsigned int height
				)
{
	int globalID = get_global_id( 0 );
	int localID = get_local_id( 0 );

	// check for work
	if(globalID < numRays)
	{
		__global Ray* r = rays + globalID;
		Ray ray = *r;

		Intersection isect;
		IntersectSceneClosest( spheres, sphereCount, &ray, &isect );

		hits[globalID] = isect;

		// Debug
		int x_coord = globalID % width;
		int y_coord = globalID / width;

		float fx = ( ( float ) x_coord / ( float ) width ) * 2.0f - 1.0f;
		float fy = ( ( float ) y_coord / ( float ) height ) * 2.0f - 1.0f;

		union Colour { float c; uchar4 components; } fcolour;

		fcolour.components = ( uchar4 )
				( ( unsigned char )( ( r->o.w / ( r->o.w + 1 ) ) * 255 ),
				  ( unsigned char )( ( r->o.w / ( r->o.w + 1 ) ) * 255 ),
				  ( unsigned char )( ( r->o.w / ( r->o.w + 1 ) ) * 255 ),
				  1 );

		output[globalID] = ( float3 )( fx, fy, fcolour.c );

	}
}
