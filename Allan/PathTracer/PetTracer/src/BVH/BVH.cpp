#include "BVH.h"
#include "SplitBVHBuilder.h"

PetTracer::BVH::BVH( Scene * scene, BuildParams const& params )
{
	mScene = scene;
	mParams = params;
	mNumNodes = 0;
	mTriangleIndexes.clear();

	if ( mParams.EnablePrints )
		std::cout << "BVH builder: " << scene->TriangleCount() << " tris, " << scene->VertexCount() << " vertices" << std::endl;

	mRoot = SplitBVHBuilder( *this, params ).Run();

	if ( mParams.EnablePrints )
		std::cout << "BVH: Scene Bounds: (" << mRoot->Bounds().Min() << ") - (" << mRoot->Bounds().Max() << ")" << std::endl << mNumNodes << " nodes" << std::endl;
}
