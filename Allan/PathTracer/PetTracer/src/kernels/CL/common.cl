


#ifndef COMMON_CL
#define COMMON_CL

#include <material.cl>

#define PI 3.14159265358979323846f
#define EPSILON 0.00003f
#define MINIMUM_THROUGHPUT 0.0f
#define NON_ZERO(x) (length(x) > EPSILON)
#define INLINE __attribute__((always_inline))
//#define INFINITY 1e20f

typedef struct _matrix4
{
	float4 m0;
	float4 m1;
	float4 m2;
	float4 m3;
} matrix4;

typedef struct _bbox
{
	float4 pmin;
	float4 pmax;
} BBox;

typedef struct _intersection
{
	// ID of a shape
	int shapeID;
	// Primitive index
	int primID;
	// Padding elements
	int padding0;
	int padding1;

	// uv - hit baricentrics, w - ray distance
	float4 uvwt;
} Intersection;

typedef struct _differentialGeometry
{
	// World space position
	float3 p;
	// Shading normal
	float3 n;
	// Geo normal
	float3 ng;
	// UVs
	float2 uv;
	// Derivatives
	float3 dpdu;
	float3 dpdv;
	float area;

	matrix4 worldToTangent;
	matrix4 tangentToWorld;

	// Maybe material later
	Material mat;

} DifferentialGeometry;

float4 makeFloat4(float x, float y, float z, float w)
{
	float4 res;
	res.x = x;
	res.y = y;
	res.z = z;
	res.w = w;
	return res;
}

float3 makeFloat3( float x, float y, float z )
{
	float3 res;
	res.x = x;
	res.y = y;
	res.z = z;
	return res;
}

float2 makeFloat2( float x, float y )
{
	float2 res;
	res.x = x;
	res.y = y;
	return res;
}


INLINE float3 GetOrthoVector( float3 n )
{
	float3 p;

    if (fabs(n.z) > 0.f) {
        float k = sqrt(n.y*n.y + n.z*n.z);
        p.x = 0; p.y = -n.z/k; p.z = n.y/k;
    }
    else {
        float k = sqrt(n.x*n.x + n.y*n.y);
        p.x = n.y/k; p.y = -n.x/k; p.z = 0;
    }

    return normalize(p);
}

INLINE matrix4 matrixFromCols(float4 c0, float4 c1, float4 c2, float4 c3)
{
    matrix4 m;
    m.m0 = makeFloat4(c0.x, c1.x, c2.x, c3.x);
    m.m1 = makeFloat4(c0.y, c1.y, c2.y, c3.y);
    m.m2 = makeFloat4(c0.z, c1.z, c2.z, c3.z);
    m.m3 = makeFloat4(c0.w, c1.w, c2.w, c3.w);
    return m;
}

INLINE matrix4 matrixFromFows(float4 c0, float4 c1, float4 c2, float4 c3)
{
    matrix4 m;
    m.m0 = c0;
    m.m1 = c1;
    m.m2 = c2;
    m.m3 = c3;
    return m;
}

INLINE matrix4 matrixFromRows3(float3 c0, float3 c1, float3 c2)
{
    matrix4 m;
    m.m0.xyz = c0; m.m0.w = 0;
    m.m1.xyz = c1; m.m1.w = 0;
    m.m2.xyz = c2; m.m2.w = 0;
    m.m3 = makeFloat4(0.f, 0.f, 0.f, 1.f);
    return m;
}

INLINE matrix4 matrixFromCols3(float3 c0, float3 c1, float3 c2)
{
    matrix4 m;
    m.m0 = makeFloat4(c0.x, c1.x, c2.x, 0.f);
    m.m1 = makeFloat4(c0.y, c1.y, c2.y, 0.f);
    m.m2 = makeFloat4(c0.z, c1.z, c2.z, 0.f);
    m.m3 = makeFloat4(0.f, 0.f, 0.f, 1.f);
    return m;
}

INLINE float3 matrixMulVector3(matrix4 m, float3 v)
{
    float3 res;
    res.x = dot(m.m0.xyz, v);
    res.y = dot(m.m1.xyz, v);
    res.z = dot(m.m2.xyz, v);
    return res;
}

#endif