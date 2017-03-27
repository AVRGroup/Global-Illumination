#ifndef SCENE_CL
#define SCENE_CL

#include <common.cl>
#include <ray.cl>

typedef struct _scene
{
	// Vertices
	__global float3 const*  vertices;
	// Normals
	__global float3 const*  normals;
	// UVs
	__global float4 const*  uvs;
	// Indices
	__global int4   const*  indices;
	// Surfaces
	// TODO
	// Material IDs
	// TODO
	// Materials
	// TODO
} Scene;


#endif