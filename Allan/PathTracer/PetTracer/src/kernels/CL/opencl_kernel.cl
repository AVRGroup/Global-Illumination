__constant int SAMPLES = 1;

#include <path.cl>
#include <primitives.cl>
#include <random.cl>


Ray CreateCamRay(const int x_coord, const int y_coord, const int width, const int height)
{
	float fx = ( float ) x_coord / ( float ) width;
	float fy = ( float ) y_coord / ( float ) height;

	float aspectRatio = ( float ) width / ( float ) height;
	float fx2 = ( fx - 0.5f ) * aspectRatio;
	float fy2 = ( fy - 0.5f );

	float3 pixelPos = ( float3 )( fx2, fy2, 0.0f );

	Ray ray;
	ray.o = ( float4 )( 0.0f, 0.1f, 2.0f, 0.0f );
	ray.d = ( float4 )(normalize( pixelPos - ray.o.xyz ), 0.0f);

	return ray;
}

bool intersectScene(__constant Sphere* spheres, __constant float4* triangles, const Ray* ray, float* t, int* sphereID, const int sphereCount, unsigned int triangleCount, DifferentialGeometry* difGeo)
{
	Intersection intersect;
	intersect.uvwt = (float4)(0.0f, 0.0f, 0.0f, INFINITY );

	int shapeType = -1;
	*sphereID = -1;

	for ( int i = 0; i < sphereCount; i++ )
	{
		if ( IntersectSphereI( &spheres[i], ray, &intersect ) )
		{
			shapeType = 1;
			*sphereID = i;
		}
	}


	for( unsigned int i = 0; i <  triangleCount; i++ )
	{
		if ( IntersectTriangleEG( ray, triangles[i*3].xyz, triangles[i*3 + 1].xyz, triangles[i*3 + 2].xyz, &intersect ) )
		{
			shapeType = 2;
			*sphereID = i;
		}
	}


	float3 v1 = ( float3 )( -0.20f, 0.40f, 0.1f );
	float3 e1 = ( float3 )( 0.4f, 0.0f, 0.0f );
	float3 e2 = ( float3 )( 0.0f, 0.0f, 0.4f );
	if ( IntersectTriangleEG( ray, v1, e1, e2, &intersect ) )
		*sphereID = 8;
	v1 = ( float3 )( 0.20f, 0.40f, 0.5f );
	e1 *= -1.0f;
	e2 *= -1.0f;
	if ( IntersectTriangleEG( ray, v1, e1, e2, &intersect ) )
		*sphereID = 8;

	difGeo->p = intersect.uvwt.w * ray->d.xyz + ray->o.xyz;

	switch ( shapeType )
	{
	case 1:
		difGeo->n = normalize( difGeo->p - spheres[*sphereID].pos );
		break;
	case 2:
		difGeo->n = normalize( cross( triangles[*sphereID * 3 + 1].xyz, triangles[*sphereID * 3 + 2].xyz ) );
		*sphereID = 6;
		break;
	};



	*t = intersect.uvwt.w;


	return *t < INFINITY;
}

float3 trace(__constant Sphere* spheres, __constant float4* triangles, __global Ray* camray, const int sphereCount, unsigned int triangleCount, unsigned int* seed0, unsigned int* seed1)
{
	Ray ray = *camray;

	float3 accumColor = ( float3 )( 0.0f, 0.0f, 0.0f );
	float accumDist = 0.0f;
	float3 mask = ( float3 )( 1.0f, 1.0f, 1.0f );

	for ( int bounces = 0; bounces < 5; bounces++ )
	{
		float t;
		int hitSphereID = 0;
		DifferentialGeometry difGeo;

		if(!intersectScene(spheres, triangles, &ray, &t, &hitSphereID, sphereCount, triangleCount, &difGeo ))
			return accumColor += mask * ( float3 )( 0.0f, 0.0f, 0.0f );

		Sphere hitSphere = spheres[hitSphereID % sphereCount];

		float3 hitPoint = difGeo.p;

		float3 normal = difGeo.n;
		float3 normalFacing = dot(normal, ray.d.xyz) < 0.0f ? normal : normal * ( -1.0f );

		float3 newDir;

		if ( hitSphere.flt == 0 )
		{
			float rand1 = 2.0f * PI * getRandom( seed0, seed1 );
			float rand2 = getRandom( seed0, seed1 );
			float rand2s = sqrt( rand2 );

			float3 w = normalFacing;
			float3 axis = fabs( w.x ) > 0.1f ? ( float3 )( 0.0f, 1.0f, 0.0f ) : ( float3 )( 1.0f, 0.0f, 0.0f );
			float3 u = normalize( cross( axis, w ) );
			float3 v = cross( w, u );

			newDir = normalize( u * cos( rand1 )*rand2s + v*sin( rand1 )*rand2s + w*sqrt( 1.0f - rand2 ) );
		} else
		{
			newDir = normalize( ray.d.xyz - normalFacing * 2.0f * dot( normalFacing, ray.d.xyz ));
		}


		ray.o = (float4)(hitPoint + normalFacing * EPSILON, 0.0f);
		ray.d = (float4)(newDir, 0.0f);

		accumDist += t;
		accumColor += mask * (hitSphere.emission / accumDist*accumDist );
		mask *= hitSphere.color;

		mask *= dot( newDir, normalFacing );
	}

	return accumColor;
}

union Colour { float c; uchar4 components; };

__kernel void render_kernel( __constant Sphere* spheres, const int width, const int height, const int sphereCount, __global float3* output,
							 const int seedt, const int iteration, __global Ray* rays, float random0, float random1, __global float3* accumColor,
							 __constant float4* triangles, unsigned int trianglesCount)
{
	const int workItemID = get_global_id( 0 );
	int x_coord = workItemID % width;
	int y_coord = workItemID / width;

	float fx = (( float ) x_coord / ( float ) width) * 2.0f - 1.0f;
	float fy = (( float ) y_coord / ( float ) height) * 2.0f - 1.0f;

	Rng rng;
	rng.val = workItemID;
	unsigned int seed0 = ( RandUint(&rng) * seedt );
	//unsigned int seed0 = (x_coord * seedt) % width;
	unsigned int seed1 = seed0 % width;

	__global Ray* camRay = rays + workItemID;

	float3 finalColor = ( float3 )( 0.0f, 0.0f, 0.0f );
	float invSamples = 1.0f / SAMPLES;

	for ( int i = 0; i < SAMPLES; i++ )
	{	
		float3 raySample = trace( spheres, triangles, camRay, 9, trianglesCount, &seed0, &seed1 );
		finalColor += raySample * invSamples;
	}
	
	float3 acColor = accumColor[workItemID] + finalColor;
	accumColor[workItemID] = acColor;

	union Colour fcolour;

	finalColor = acColor / iteration;
	
	fcolour.components = ( uchar4 )
		( ( unsigned char )( ( finalColor.x / ( finalColor.x + 1 ) ) * 255 ),
		  ( unsigned char )( ( finalColor.y / ( finalColor.y + 1 ) ) * 255 ),
		  ( unsigned char )( ( finalColor.z / ( finalColor.z + 1 ) ) * 255 ),
		  1);

	output[workItemID] = (float3)(fx, fy, fcolour.c);
}