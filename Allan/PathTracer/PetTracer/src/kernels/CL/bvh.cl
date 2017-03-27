#ifndef BVH_CL
#define BVH_CL

#include <path.cl>
#include <primitives.cl>

#define STARTIDX(x)     (((int)(x->pmin.w)))
#define LEAFNODE(x)     (((x).pmin.w) != -1.f)

typedef BBox BVHNode;

typedef struct
{
	// BVH structure
	__global BVHNode const*		nodes;
	// Scene Positional data
	__global float3  const*		vertices;
	// Scene Indices
	__global int4    const*		faces;

} SceneData;

void IntersectLeafClosest(
	SceneData const* scenedata,
	BVHNode const* node,
	Ray const* r,
	Intersection* inter )
{
	float3 v1, v2, v3;
	int4 face;

	int start = STARTIDX( node );
	face = scenedata->faces[start];
	v1 = scenedata->vertices[face.x];
	v2 = scenedata->vertices[face.y];
	v3 = scenedata->vertices[face.z];

	if( IntersectTriangle(r, v1, v2, v3, inter) )
	{
		inter->primID = start;
		inter->shapeID = face.w;
		//inter->uvwt.xyz = normalize( cross(v2 - v1, v3 - v1) );
	}
}

void IntersectSceneClosest(SceneData const* scenedata, Ray const* r, Intersection* inter )
{
	const float3 invDir = makeFloat3(1.0f, 1.0f, 1.0f) / (r->d.xyz);

	inter->uvwt = makeFloat4( 0.0f, 0.0f, 0.0f, r->o.w );
	inter->shapeID = -1;
	inter->primID = -1;

	int idx = 0;

	while(idx != -1)
	{
		BVHNode node = scenedata->nodes[idx];
		// If hits the AABB, check the childs
		if ( IntersectBox( r, invDir, node, inter->uvwt.w ) )
		{
			// Debug
			//inter->uvwt.x += 0.01f;
			// If its a leaf, intersect its triangles
			if ( LEAFNODE( node ) )
			{
				IntersectLeafClosest( scenedata, &node, r, inter );
				idx = ( int ) ( node.pmax.w );
			}
			// else, traverse child
			else
			{
				++idx;
			}
		}
		// If not, go back to last AABB hit right child
		else
		{
			idx = ( int ) ( node.pmax.w );
		}
	};

}

#endif