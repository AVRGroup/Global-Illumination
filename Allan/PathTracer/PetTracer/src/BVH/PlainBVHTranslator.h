#pragma once

#include "BVH.h"

namespace PetTracer
{
	class PlainBVHTranslator
	{
	public:
		PlainBVHTranslator()
			: mNodeCount(0),
			  mRoot(0)
		{ }


		struct Node
		{
			AABB bounds;
		};


		void Process( BVH const& bvh );


		std::vector<Node> mNodes;
		std::vector<int32> mExtra;
		std::vector<int32> mRoots;

		int32 mRoot;
		int32 mNodeCount;
	private:
		int32 ProcessNode( BVHNode const* node );


	};
}