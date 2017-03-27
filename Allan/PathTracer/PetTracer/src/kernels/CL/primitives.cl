
#ifndef PRIMITIVES_CL
#define PRIMITIVES_CL

#include <common.cl>
#include <ray.cl>
#include <path.cl>

typedef struct Sphere
{
	float radius;
	uint  flt;
	float padding0;
	float padding1;
	float4 pos;
	float4 color;
	float4 emission;
} Sphere;

float IntersectSphere( __global Sphere* sphere, const Ray* ray )
{
	float3 rayToCenter = sphere->pos.xyz - ray->o.xyz;

	float b = dot( rayToCenter, ray->d.xyz );
	float c = dot( rayToCenter, rayToCenter ) - sphere->radius*sphere->radius;
	float disc = b * b - c;

	if ( disc < 0.0f ) return 0.0f;
	else disc = sqrt( disc );

	float t = ( b - disc );
	if ( t > EPSILON ) return t;
	t = ( b + disc );
	if ( t > EPSILON ) return t;

	return 0.0f;
}

int IntersectSphereI( __global Sphere* sphere, const Ray* ray, Intersection* intersec )
{
	float3 rayToCenter = sphere->pos.xyz - ray->o.xyz;

	float b = dot( rayToCenter, ray->d.xyz );
	float c = dot( rayToCenter, rayToCenter ) - sphere->radius*sphere->radius;
	float disc = b * b - c;

	if ( disc < 0.0f ) return 0;
	else disc = sqrt( disc );

	float t = ( b - disc );
	if ( t > EPSILON && t < intersec->uvwt.w )
	{
		intersec->uvwt = ( float4 )( 0.0f, 0.0f, 0.0f, t );
		return 1;
	}
	t = ( b + disc );
	if ( t > EPSILON && t < intersec->uvwt.w )
	{
		intersec->uvwt = ( float4 )( 0.0f, 0.0f, 0.0f, t );
		return 1;
	}

	return 0;
}

int IntersectTriangleEG(Ray const* r, float3 v1, float3 e1, float3 e2, Intersection* intersec )
{
	const float3 d = r->o.xyz - v1;
	const float3 s1 = cross( r->d.xyz, e2 );
	const float3 s2 = cross( d, e1 );
	const float invd = native_recip( dot( e1, s1 ) );
	const float b1 = dot( d, s1 ) * invd;
	const float b2 = dot( r->d.xyz, s2 ) * invd;
	const float temp = dot( e2, s2 ) * invd;

	if(b1 < 0.0f || b1 > 1.0f || b2 < 0.0f || b2 > 1.0f || b1 + b2 > 1.0f
		|| temp < 0.0f || temp > intersec->uvwt.w)
	{
		return 0;
	}
	else
	{
		intersec->uvwt = ( float4 )( b1, b2, 0, temp );
		return 1;
	}
}

int IntersectTriangle( Ray const* r, float3 v1, float3 v2, float3 v3, Intersection* isect )
{
	const float3 e1 = v2 - v1;
	const float3 e2 = v3 - v1;
	const float3 s1 = cross( r->d.xyz, e2 );
	const float  invd = native_recip( dot( s1, e1 ) );
	const float3 d = r->o.xyz - v1;
	const float  b1 = dot( d, s1 ) * invd;
	const float3 s2 = cross( d, e1 );
	const float  b2 = dot( r->d.xyz, s2 ) * invd;
	const float temp = dot( e2, s2 ) * invd;

	if ( b1 < 0.f || b1 > 1.f || b2 < 0.f || b1 + b2 > 1.f
		|| temp < 0.f || temp > isect->uvwt.w )
	{
		return 0;
	}
	else
	{
		isect->uvwt = makeFloat4( isect->uvwt.x, b2, 0.f, temp );
		return 1;
	}
}

int IntersectBox( Ray const* r, float3 invdir, BBox box, float maxt )
{
	const float3 f = ( box.pmax.xyz - r->o.xyz ) * invdir;
	const float3 n = ( box.pmin.xyz - r->o.xyz ) * invdir;

	const float3 tmax = max( f, n );
	const float3 tmin = min( f, n );

	const float t1 = min( min( tmax.x, min( tmax.y, tmax.z ) ), maxt );
	const float t0 = max( max( tmin.x, max( tmin.y, tmin.z ) ), 0.f );

	return ( t1 >= t0 ) ? 1 : 0;
}


#endif
