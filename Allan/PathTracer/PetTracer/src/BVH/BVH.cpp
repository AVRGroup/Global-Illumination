#include "BVH.h"
#include "SplitBVHBuilder.h"

PetTracer::BVH::BVH( Scene * scene, const BuildParams & params )
{
	mScene = scene;
	mParams = params;

	if ( mParams.EnablePrints )
		std::cout << "BVH builder: " << scene->TriangleCount() << " tris, " << scene->VertexCount() << " vertices" << std::endl;

	mRoot = SplitBVHBuilder( *this, params ).Run();
}
