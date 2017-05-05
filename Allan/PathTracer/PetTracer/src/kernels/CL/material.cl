#ifndef MATERIAL_CL
#define MATERIAL_CL

/*enum MaterialType
{
	mtDielectric,
	mtMetal,
	mtEmissive
}*/

typedef struct _material
{
	union
	{
		float4 albedo;
		float4 specular;
	};

	float4 emissive;

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
	
	union
	{
		float fresnel;
		float metallic;
	};

} Material;


#endif