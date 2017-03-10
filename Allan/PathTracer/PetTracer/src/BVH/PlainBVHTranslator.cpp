#include "PlainBVHTranslator.h"

namespace PetTracer
{
	void PlainBVHTranslator::Process( BVH const& bvh )
	{
		mNodeCount = 0;
		int newSize = bvh.NodeCount();
		mNodes.resize( newSize );
		mExtra.resize( newSize );

		if ( !bvh.Root() )
			return;

		int32 rootIdx = 0;

		ProcessNode( bvh.Root() );

		mNodes[rootIdx].bounds.Max().w = -1;

		for ( int32 i = rootIdx; i < (int32) mNodes.size(); ++i )
		{
			// If its an inner node
			if ( mNodes[i].bounds.Min().w != -1.0f )
			{
				mNodes[i + 1].bounds.Max().w = mNodes[i].bounds.Min().w;
				mNodes[( int32 ) ( mNodes[i].bounds.Min().w )].bounds.Max().w = mNodes[i].bounds.Max().w;
			}
		}

		for ( int32 i = rootIdx; i < ( int32 ) mNodes.size(); i++ )
		{
			// If its an leaf node
			if ( mNodes[i].bounds.Min().w == -1 )
			{
				mNodes[i].bounds.Min().w = ( float ) mExtra[i];
			}
			else
			{
				mNodes[i].bounds.Min().w = -1.0f;
			}
		}


	}

	int32 PlainBVHTranslator::ProcessNode( BVHNode const* n )
	{
		int32 idx = mNodeCount;
		Node& node = mNodes[mNodeCount];
		node.bounds.Grow(n->Bounds());
		int& extra = mExtra[mNodeCount++];

		if ( n->IsLeaf() )
		{
			LeafNode* leaf = ( LeafNode* ) n;
			int32 startIdx = leaf->Low();
			extra = startIdx;
			node.bounds.Min().w = -1.0f;
		}
		else
		{
			InnerNode* inner = ( InnerNode* ) n;
			ProcessNode( inner->GetChildNode( 1 ) );
			node.bounds.Min().w = ( float ) ProcessNode( inner->GetChildNode( 0 ) );
		}

		return idx;
	}
}