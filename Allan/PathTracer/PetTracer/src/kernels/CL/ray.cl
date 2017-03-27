#ifndef RAY_CL
#define RAY_CL

#include "common.cl"

typedef struct _ray
{
	// xyz - origin, w - max range
	float4 o;
	// xyz - direction, w - time
	float4 d;

	int2 extra;
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

void Ray_SetInactive(__global Ray* r)
{
	r->extra.y = 0;
}

void Ray_SetExtra(__global Ray* r, float2 extra)
{
	r->padding = extra;
}

float2 Ray_GetExtra(__global Ray* r)
{
	return r->padding;
}



#endif