
#ifndef PRIMITIVES_CL
#define PRIMITIVES_CL

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

#endif
