#pragma once
#include "BVH.h"
#include "../Timer.h"

namespace PetTracer
{
	//------------------------------------------------------------------------

	class SplitBVHBuilder
	{
	private:
		enum
		{
			MaxDepth        = 64,
			MaxSpatialDepth = 48,
			NumSpatialBins  = 128,
		};

		struct Reference
		{
			int32                 triIdx;
			AABB                bounds;

			Reference( void ) : triIdx( -1 ) { }
		};

		struct NodeSpec
		{
			int32                 numRef;
			AABB                bounds;

			NodeSpec( void ) : numRef( 0 ) { }
		};

		struct ObjectSplit
		{
			float                 sah;
			int32                 sortDim;
			int32                 numLeft;
			AABB                leftBounds;
			AABB                rightBounds;

			ObjectSplit( void ) : sah( FLT_MAX ), sortDim( 0 ), numLeft( 0 ) { }
		};

		struct SpatialSplit
		{
			float                 sah;
			int32                 dim;
			float                 pos;

			SpatialSplit( void ) : sah( FLT_MAX ), dim( 0 ), pos( 0.0f ) { }
		};

		struct SpatialBin
		{
			AABB                bounds;
			int32                 enter;
			int32                 exit;
		};

	public:
		SplitBVHBuilder( BVH& bvh, const BuildParams& params );
		~SplitBVHBuilder( void );

		BVHNode*                Run( void );

	private:
		static int              sortCompare( const void* a, const void* b );
		static void             sortSwap( void* data, int32 idxA, int32 idxB );

		BVHNode*                buildNode( NodeSpec spec, int32 level, float progressStart, float progressEnd );
		BVHNode*                createLeaf( const NodeSpec& spec );

		ObjectSplit             findObjectSplit( const NodeSpec& spec, float nodeSAH );
		void                    performObjectSplit( NodeSpec& left, NodeSpec& right, const NodeSpec& spec, const ObjectSplit& split );

		SpatialSplit            findSpatialSplit( const NodeSpec& spec, float nodeSAH );
		void                    performSpatialSplit( NodeSpec& left, NodeSpec& right, const NodeSpec& spec, const SpatialSplit& split );
		void                    splitReference( Reference& left, Reference& right, const Reference& ref, int32 dim, float pos );

	private:
		SplitBVHBuilder( const SplitBVHBuilder& ); // forbidden
		SplitBVHBuilder&        operator=           ( const SplitBVHBuilder& ); // forbidden

	private:
		static SplitBVHBuilder* sPointer;

	private:
		BVH&                    mBVH;
		const BuildParams&      mParams;

		std::vector<Reference>  mReferenceStack;
		float                   mMinOverlap;
		std::vector<AABB>       mRightBounds;
		int32                   mSortDim;
		SpatialBin              mBins[3][NumSpatialBins];

		Timer<milliseconds>     mProgressTimer;
		int32                   mNumDuplicates;
	};

	//------------------------------------------------------------------------
}
