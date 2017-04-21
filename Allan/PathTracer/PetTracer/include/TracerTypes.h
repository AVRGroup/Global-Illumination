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
			int throughput[4];
			int volume;
			int flags;
			int active;
			int extra1;
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

		typedef struct _material
		{
			/*struct _material( cl_float4 spe_alb, cl_float4 _emissive, float _roughness, float _metallic )
			{
				albedo = spe_alb;
				emissive = _emissive;
				roughness = _roughness;
				metallic = _metallic;
			}*/

			union
			{
				cl_float4 albedo;
				cl_float4 specular;
			};

			cl_float4 emissive;

			union
			{
				int albedoTexID;
				int specularTexID;
			};

			int normalTexID;

			union
			{
				float ior;
				float roughness;
			};

			float metallic;

		} Material;
	}
}