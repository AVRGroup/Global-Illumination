#pragma once

#include "BVH.h"

#include <vector>

namespace PetTracer
{
	class SplitBVHBuilder
	{
	private:
		enum
		{
			MaxDepth		= 64,
			MaxSpatialDepth = 48,
			NumSpatialBins	= 128
		};

		struct Reference
		{
			AABB	Bounds;

			void		  TriIdx( int32 idx ) { Bounds.Low() = idx; };
			inline int32& TriIdx() { return Bounds.Low(); };
			inline int32  TriIdx() const { return Bounds.Low(); };

			Reference() { TriIdx(-1); }
			Reference( const AABB& aabb ) : Bounds( aabb ) { }
			//const Reference operator=( const Reference& r ) { return Reference( r.Bounds ); }
		};

		struct NodeSpecification
		{
			AABB	Bounds;

			void		  NumRef( int32 ref ) { Bounds.Low() = ref; };
			inline int32& NumRef() { return Bounds.Low(); };
			inline int32  NumRef() const { return Bounds.Low(); };

			NodeSpecification() { NumRef(0); }
		};

		struct ObjectSplit
		{
			float	SAH;
			int32	SortDim;
			int32	NumLeft;
			AABB	LeftBounds;
			AABB	RightBounds;

			ObjectSplit() : SAH(FLT_MAX), SortDim(0), NumLeft(0) { }
		};

		struct SpatialSplit
		{
			float	SAH;
			int32	Dim;
			float	Pos;

			SpatialSplit() : SAH(FLT_MAX), Dim(0), Pos(0.0f) { }
		};

		struct SpatialBin
		{
			AABB	Bounds;
			int32	enter;
			int32	exit;
		};

	public:
		SplitBVHBuilder( BVH& bvh, BuildParams const& params );

		BVHNode* Run();

	private:
		BVHNode*		BuildNode(NodeSpecification spec, int level, float progressStart, float progressEnd);
		BVHNode*		CreateLeaf( NodeSpecification& spec );


		ObjectSplit		FindObjectSplit ( const NodeSpecification& spec, float nodeSAH );
		void			PerformObjectSplit( NodeSpecification& left, NodeSpecification& right, const NodeSpecification& spec, const ObjectSplit& split );

		SpatialSplit	FindSpatialSplit( const NodeSpecification& spec, float nodeSAH );
		void			PerformSpatialSplit( NodeSpecification& left, NodeSpecification& right, const NodeSpecification& spec, const SpatialSplit& split );


		void			SplitReference( Reference & left, Reference & right, const Reference & ref, int32 dim, float pos );

	private:
		static int SortCompare( const void* refA, const void* refB );

	private:
		BVH&					mBVH;
		const BuildParams&		mParams;

		std::vector<Reference>	mReferenceStack;
		float					mMinOverlap;
		std::vector<AABB>		mRightBounds;
		int32					mSortDim;
		SpatialBin				mBins[3][NumSpatialBins];

		int32					mNumDuplicates;

		static SplitBVHBuilder*	sPointer;
	};
}