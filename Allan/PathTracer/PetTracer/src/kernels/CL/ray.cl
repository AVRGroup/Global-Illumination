#ifndef RAY_CL
#define RAY_CL

#include "common.cl"

typedef struct _ray
{
	// xyz - origin, w - max range
	float4 o;
	// xyz - direction, w - time
	float4 d;

	int2   extra;
	float2 padding;
} Ray;

void Ray_SetPixel(__global Ray* r, int pixId)
{
	r->extra.x = pixId;
}

int Ray_GetPixel(__global Ray const* r)
{
	return r->extra.x;
}

int Ray_IsActive( Ray const* r )
{
	return r->extra.y;
}

void Ray_SetInactive(__global Ray* r)
{
	r->extra.y = 0;
}

void Ray_SetExtra(__global Ray* r, float2 extra)
{
	r->padding = extra;
}

float2 Ray_GetExtra(__global Ray const* r)
{
	return r->padding;
}

void Ray_Init(__global Ray* r, float3 o, float3 d, float maxt, float time, int mask)
{
	r->o.xyz = o;
	r->d.xyz = d;
	r->o.w = maxt;
	r->d.w = time;
	r->extra.x = mask;
	r->extra.y = 0xFFFFFFFF;
}



#endif