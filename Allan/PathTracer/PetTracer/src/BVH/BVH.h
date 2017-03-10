#pragma once

#include "math/MathUtils.h"
#include "BVHNode.h"
#include "../Scene/Scene.h"

namespace PetTracer
{
	struct Stats
	{
		Stats() { };
		void Clear() { memset( this, 0, sizeof( Stats ) ); };

		float	SAHCost;
		uint32	BranchingFactor;
		uint32	NumInnerNodes;
		uint32	NumLeafNodes;
		uint32	NumChildNodes;
		uint32  NumTriangles;
	};

	struct BuildParams
	{
		Stats*	stats;
		float	SplitAlpha;
		bool	EnablePrints;

		float	SAHNodeCost;
		float	SAHTriangleCost;
		int32	TriangleBatchSize;
		int32	NodeBatchSize;
		int32	MinLeafSize;
		int32	MaxLeafSize;

		inline float	TriangleCost( int32 n ) const { return ( RoundToTriangleBatchSize( n ) * SAHTriangleCost ); }
		inline float	NodeCost( int32 n ) const { return ( RoundToNodeBatchSize( n ) * SAHNodeCost ); }

		BuildParams()
		{
			stats = NULL;
			SplitAlpha = 1.0e-5f;
			SAHNodeCost = 1.0f;
			SAHTriangleCost = 4.0f;
			NodeBatchSize = 1;
			TriangleBatchSize = 1;
			MinLeafSize = 1;
			MaxLeafSize = 0x7FFFFFF;
			EnablePrints = true;
		}

		~BuildParams()
		{
			delete stats;
		}

	private:
		inline int32 RoundToTriangleBatchSize( int32 n ) const { return ( ( n + TriangleBatchSize - 1 ) / TriangleBatchSize ) * TriangleBatchSize; }
		inline int32 RoundToNodeBatchSize( int32 n ) const { return ( ( n + NodeBatchSize - 1 ) / NodeBatchSize ) * NodeBatchSize; }

	};

	class BVH
	{
	public:
		BVH( Scene* scene, BuildParams const& params);
		~BVH() { if ( mRoot ) mRoot->DeleteSubTree(); }

		inline Scene& GetScene() { return *mScene; }

		inline std::vector<int32>&		 TriangleIndices()		 { return mTriangleIndexes; }
		inline const std::vector<int32>& TriangleIndices() const { return mTriangleIndexes; }

		inline uint32&					 NodeCount()              { return mNumNodes; }
		inline const uint32&			 NodeCount() const        { return mNumNodes; }

		inline const BVHNode*			 Root() const	{ return mRoot; }
		inline BVHNode*					 Root()			{ return mRoot; }

	private:
		BVHNode*			mRoot;
		std::vector<int32>	mTriangleIndexes;

		uint32				mNumNodes;

		Scene*				mScene;
		BuildParams			mParams;
	};
}