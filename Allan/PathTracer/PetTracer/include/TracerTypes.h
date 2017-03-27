#pragma once

#include <CL\cl.h>

namespace PetTracer
{
	namespace CLTypes
	{
		typedef struct _ray
		{
			// xyz - origin, w - max range
			cl_float4 o;
			// xyz - direction, w - time
			cl_float4 d;

			cl_int2 extra;
			cl_float2 padding;
		} Ray;

		typedef struct _path
		{
			cl_float3 throughput;
			cl_int volume;
			cl_int flags;
			cl_int active;
			cl_int extra1;
		} Path;

		struct BBox
		{
			cl_float4 pmin;
			cl_float4 pmax;
		};

		typedef struct _intersection
		{
			int shapeID;
			int pimID;
			int padding0;
			int padding1;

			cl_float4 uvwt;
		} Intersection;
	}
}