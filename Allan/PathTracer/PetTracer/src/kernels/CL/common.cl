


#ifndef COMMON_CL
#define COMMON_CL

#define PI 3.14159265358979323846f
#define EPSILON 0.00003f
#define MINIMUM_THROUGHPUT 0.0f
//#define INFINITY 1e20f

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
	// Maybe material later

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

#endif