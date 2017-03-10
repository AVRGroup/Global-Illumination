__constant int SAMPLES = 1;

#include <path.cl>
#include <primitives.cl>
#include <random.cl>
#include <bvh.cl>


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

bool intersectScene( __global Sphere* spheres, SceneData const* scenedata, const Ray* ray, float* t, int* sphereID, const int sphereCount, DifferentialGeometry* difGeo)
{
	Intersection intersect;
	intersect.uvwt = (float4)(0.0f, 0.0f, 0.0f, INFINITY );

	/*BBox meshBB;
	meshBB.pmin = ( float4 )( -0.10f, -0.10f, -0.10f, 0.0f );
	meshBB.pmax = ( float4 )(  0.10f,  0.10f,  0.10f, 0.0f );*/

	int shapeType = 2;
	*sphereID = 1;

	ray->o.w = intersect.uvwt.w;
	IntersectSceneClosest( scenedata, ray, &intersect );
	*sphereID = intersect.shapeID;

	/*for ( int i = 0; i < sphereCount; i++ )
	{
		if ( IntersectSphereI( &spheres[i], ray, &intersect ) )
		{
			shapeType = 1;
			*sphereID = i;
		}
	}*/

	float3 v1 = ( float3 )( -0.20f, 0.92f, -0.225f );
	float3 e1 = ( float3 )( 0.4f, 0.0f, 0.0f );
	float3 e3 = ( float3 )( 0.0f, 0.0f, 0.4f );
	if ( IntersectTriangleEG( ray, v1, e1, e3, &intersect ) )
		*sphereID = 8;
	v1 = ( float3 )( 0.20f, 0.92f, 0.175f );
	e1 *= -1.0f;
	e3 *= -1.0f;
	if ( IntersectTriangleEG( ray, v1, e1, e3, &intersect ) )
		*sphereID = 8;

	difGeo->p = intersect.uvwt.w * ray->d.xyz + ray->o.xyz;

	switch ( shapeType )
	{
	case 1:
		difGeo->n = normalize( difGeo->p - spheres[*sphereID].pos.xyz );
		break;
	case 2:
		difGeo->n = intersect.uvwt.xyz;
		//*sphereID = 5;
		break;
	};



	*t = intersect.uvwt.w;


	return *t < INFINITY;
}

float3 trace(__global Sphere* spheres, SceneData const* scenedata, __global Ray* camray, const int sphereCount, unsigned int* seed0, unsigned int* seed1)
{
	Ray ray = *camray;

	float3 accumColor = ( float3 )( 0.0f, 0.0f, 0.0f );
	float accumDist = 0.0f;
	float3 mask = ( float3 )( 1.0f, 1.0f, 1.0f );

	for ( int bounces = 0; bounces < 2; bounces++ )
	{
		float t;
		int hitSphereID = 0;
		DifferentialGeometry difGeo;

		if(!intersectScene(spheres, scenedata, &ray, &t, &hitSphereID, sphereCount, &difGeo ))
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
		ray.d = (float4)(newDir, ray.d.w);


		if ( mask.x <= 0.001f ) mask.x = 0.0f;
		if ( mask.y <= 0.001f ) mask.y = 0.0f;
		if ( mask.z <= 0.001f ) mask.z = 0.0f;

		accumDist += t;
		accumColor += mask * (hitSphere.emission.xyz/* / accumDist*accumDist */);
		mask *= hitSphere.color.xyz;

		mask *= dot( newDir, normalFacing );
	}

	return accumColor;
}

union Colour { float c; uchar4 components; };

__kernel void render_kernel( __global Sphere* spheres, const int width, const int height, const int sphereCount, __global float3* output,
							 const int seedt, const int iteration, __global Ray* rays, float random0, float random1, __global float3* accumColor,
							 __global float4* vertices, __global int4* faces, __global BBox* bvhnodes)
{
	const int workItemID = get_global_id( 0 );
	int x_coord = workItemID % width;
	int y_coord = workItemID / width;

	float fx = (( float ) (x_coord + 0.0001f) / ( float ) width) * 2.0f - 1.0f;
	float fy = (( float ) (y_coord + 0.0001f) / ( float ) height) * 2.0f - 1.0f;

	// Organie scene
	SceneData scenedata = { bvhnodes, vertices, faces };

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
		float3 raySample = trace( spheres, &scenedata, camRay, 9, &seed0, &seed1 );
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