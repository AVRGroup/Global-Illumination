


#ifndef PATH_CL
#define PATH_CL

#define PI 3.14159265358979323846f
#define EPSILON 0.00003f
//#define INFINITY 1e20f

typedef struct _ray
{
	// xyz - origin, w - max range
	float4 o;
	// xyz - direction, w - time
	float4 d;

	int2 extra;
	float2 padding;
} Ray;

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

#endif