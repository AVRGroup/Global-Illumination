#ifndef SCENE_CL
#define SCENE_CL

#include <common.cl>
#include <ray.cl>
#include <material.cl>

typedef struct _scene
{
	// Vertices
	__global float3		const*  vertices;
	// Normals
	__global float3		const*  normals;
	// UVs
	__global float2		const*  uvs;
	// Indices
	__global int4		const*  indices;
	// Surfaces
	// TODO
	// Material IDs
	// TODO
	// Materials
	__global Material	const* materials;
} Scene;

void DifferentialGeometry_Fill(
	// Scene
	Scene const* scene,
	// Intersections
	Intersection const* isect,
	// Differenctial geometry
	DifferentialGeometry* diffgeo
)
{
	// Determine shape and polygon
	int shapeID = isect->shapeID;
	int primID = isect->primID;

	// Get barycentrics
	float2 uv = isect->uvwt.xy;

	// Fetch indices
	int4 Idx = scene->indices[primID];

	// Fetch normals
	float3 n0 = scene->normals[Idx.x];
	float3 n1 = scene->normals[Idx.y];
	float3 n2 = scene->normals[Idx.z];

	// Fetch positions
	float3 v0 = scene->vertices[Idx.x];
	float3 v1 = scene->vertices[Idx.y];
	float3 v2 = scene->vertices[Idx.z];

	// Fetch UVs
	float2 uv0 = scene->uvs[Idx.x];
	float2 uv1 = scene->uvs[Idx.y];
	float2 uv2 = scene->uvs[Idx.z];

	// Calculate barycentric position and normal
	diffgeo->n = normalize( ( 1.0f - uv.x - uv.y ) * n0 + uv.x * n1 + uv.y * n2 );
	diffgeo->p  = ( 1.0f - uv.x - uv.y ) *  v0 + uv.x *  v1 + uv.y * v2 ;
	diffgeo->uv = ( 1.0f - uv.x - uv.y ) * uv0 + uv.x * uv1 + uv.y * uv2;

	float3 ng = cross( v1 - v0, v2 - v0 );
	diffgeo->area = 0.5f * length( ng );
	diffgeo->ng = normalize( ng );

	if ( dot( diffgeo->ng, diffgeo->n ) < 0.f )
		diffgeo->ng = -diffgeo->ng;

	// Get material at shading point
	diffgeo->mat = scene->materials[shapeID];

	diffgeo->dpdu = GetOrthoVector( diffgeo->n );
	diffgeo->dpdv = cross( diffgeo->n, diffgeo->dpdu );
	
}

INLINE void DifferentialGeometry_CalculateTangentTransforms(DifferentialGeometry* diffgeo)
{
	diffgeo->worldToTangent = matrixFromRows3(diffgeo->dpdu, diffgeo->n, diffgeo->dpdv);

	diffgeo->tangentToWorld = matrixFromCols3(diffgeo->worldToTangent.m0.xyz,
		diffgeo->worldToTangent.m1.xyz, diffgeo->worldToTangent.m2.xyz);


}

float3 DifferentialGeometry_ToTangentSpace(
	// Geometry
	DifferentialGeometry const* dg,
	// Vector
	float3 vetWorldSpace
)
{
	float3 vetTangentSpace = makeFloat3( dot(dg->dpdu, vetWorldSpace ), dot( dg->n, vetWorldSpace ), dot(dg->dpdv, vetWorldSpace) );
	return vetTangentSpace;
}

float3 DifferentialGeometry_ToWorldSpace(
	// Geometry
	DifferentialGeometry const* dg,
	// Vector
	float3 vetTangentSpace
)
{
	float3 vetWorldSpace = dg->dpdu * vetTangentSpace.x + dg->dpdv * vetTangentSpace.z + dg->n * vetTangentSpace.y;
	return vetWorldSpace;
}


#endif