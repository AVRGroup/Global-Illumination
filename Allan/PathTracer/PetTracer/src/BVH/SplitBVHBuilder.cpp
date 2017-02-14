#include "SplitBVHBuilder.h"

#include <algorithm>

template <typename T>
T SwapRemove( std::vector<T>& Vector, size_t idx )
{
	if ( idx < Vector.size() )
	{
		T old = Vector[idx];

		Vector[idx] = Vector[Vector.size() - 1];
		Vector.pop_back();

		return old;
	}
	return T();
};

namespace PetTracer
{
	SplitBVHBuilder*	SplitBVHBuilder::sPointer = NULL;

	SplitBVHBuilder::SplitBVHBuilder( BVH & bvh, BuildParams const & params )
		: mBVH(bvh),
		  mParams(params),
		  mMinOverlap(0.0f),
		  mSortDim(-1)
	{
	}

	BVHNode* SplitBVHBuilder::Run()
	{
		int4 const*		tris  = mBVH.GetScene().TrianglesIndexesPtr();
		float4 const*	verts = mBVH.GetScene().VerticesPositionPtr();

		NodeSpecification rootSpec;
		rootSpec.NumRef() = mBVH.GetScene().TriangleCount();
		mReferenceStack.resize( rootSpec.NumRef() );

		for ( int32 i = 0; i < rootSpec.NumRef(); i++ )
		{
			// Each reference refers to one triangle
			mReferenceStack[i].TriIdx( i );
			// Build the bounding box for each triangle
			for ( int32 j = 0; j < 3; j++ )
				mReferenceStack[i].Bounds.Grow( verts[tris[i][j]] );

			// Update the root bounding box with the triangle BB
			rootSpec.Bounds.Grow( mReferenceStack[i].Bounds );
		}

		mMinOverlap = rootSpec.Bounds.Area() * mParams.SplitAlpha;
		mRightBounds.resize( max( rootSpec.NumRef(), NumSpatialBins ) - 1 );
		mNumDuplicates = 0;

		// Build the tree recursively
		BVHNode* root = BuildNode( rootSpec, 0, 0.0f, 1.0f );
		mBVH.TriangleIndices().clear();

		if ( mParams.EnablePrints )
			std::cout << "SplitBVHBuilder: progress 100%, duplicates " << mNumDuplicates / ( float ) mBVH.GetScene().TriangleCount() * 100.0f << "% \n";

		return root;
	}


	BVHNode * SplitBVHBuilder::BuildNode( NodeSpecification spec, int level, float progressStart, float progressEnd )
	{
		// Future progress log here
		if(mParams.EnablePrints)


		// Remove degenerate AABB
		{
			int64 firstRef = mReferenceStack.size() - spec.NumRef();
			for ( int64 i = mReferenceStack.size() - 1; i >= firstRef; i-- )
			{
				float3 size = mReferenceStack[i].Bounds.Max() - mReferenceStack[i].Bounds.Min();
				if ( min( size ) < 0.0f || sum( size ) == max( size ) )
					SwapRemove(mReferenceStack, i );

			}
			spec.NumRef(static_cast<int32>(mReferenceStack.size() - firstRef));
		}

		// If it is small enough or too deep, than create a leaf
		if ( spec.NumRef() <= mParams.MinLeafSize || level > MaxDepth )
			return CreateLeaf( spec );

		// Find split candidates
		float area = spec.Bounds.Area();
		float leafSAH = area * mParams.TriangleCost( spec.NumRef() );
		float nodeSAH = area * mParams.NodeCost( 2 );
		ObjectSplit object = FindObjectSplit( spec, nodeSAH );

		SpatialSplit spatial;
		if ( level < MaxSpatialDepth )
		{
			AABB overlap = object.LeftBounds;
			overlap.Intersect( object.RightBounds );
			if ( overlap.Area() >= mMinOverlap )
				spatial = FindSpatialSplit( spec, nodeSAH );
		}

		// If leafSAH is the lowest, than create a leaf

		float minSAH = min( leafSAH, object.SAH, spatial.SAH );
		if ( minSAH == leafSAH && spec.NumRef() <= mParams.MaxLeafSize )
			return CreateLeaf( spec );

		// Perform split
		NodeSpecification left, right;
		if ( minSAH == spatial.SAH )
			PerformSpatialSplit(left, right, spec, spatial);
		if ( !left.NumRef() || !right.NumRef() )
			PerformObjectSplit( left, right, spec, object );

		// Create inner node
		mNumDuplicates += left.NumRef() - right.NumRef() - spec.NumRef();
		float progressMid = lerp(progressStart, progressEnd, right.NumRef() / static_cast<float>(left.NumRef() + right.NumRef()));
		BVHNode* rightNode = BuildNode( right, level + 1, progressStart, progressMid );
		BVHNode* leftNode = BuildNode( left, level + 1, progressMid, progressEnd );
		return new InnerNode(spec.Bounds, leftNode, rightNode);
	}

	BVHNode * SplitBVHBuilder::CreateLeaf( NodeSpecification & spec )
	{
		std::vector<int32>& tris = mBVH.TriangleIndices();
		for ( int32 i = 0; i < spec.NumRef(); i++ )
		{
			tris.push_back( mReferenceStack[mReferenceStack.size() - 1].TriIdx() );
			mReferenceStack.pop_back();
		}

		return new LeafNode(spec.Bounds, static_cast<int32>(tris.size() - spec.NumRef()), static_cast<int32>(tris.size()));
	}

	SplitBVHBuilder::ObjectSplit SplitBVHBuilder::FindObjectSplit( const NodeSpecification & spec, float nodeSAH )
	{
		ObjectSplit split;
		const Reference* refPtr = &mReferenceStack[mReferenceStack.size() - spec.NumRef()];
		float bestTieBreak = FLT_MAX;

		// Prepare for sort function
		sPointer = this;

		// Sort along each direction
		for ( mSortDim = 0; mSortDim < 3; mSortDim++ )
		{
			/// NEED SORT HERE
			std::qsort( &mReferenceStack[mReferenceStack.size() - spec.NumRef()], spec.NumRef(), sizeof( Reference ), SplitBVHBuilder::SortCompare );

			// Sweep right to left and determine bounds
			AABB rightBounds;
			for(int32 i = spec.NumRef() -1; i > 0; i++)
			{
				rightBounds.Grow( refPtr[i].Bounds );
				mRightBounds[i - 1] = rightBounds;
			}

			// Sweep left to right and select lowest SAH
			AABB leftBounds;
			for ( int32 i = 1; i < spec.NumRef(); i++ )
			{
				leftBounds.Grow( refPtr[i - 1].Bounds );
				float sah = nodeSAH + leftBounds.Area() * mParams.TriangleCost( i ) + mRightBounds[i - 1].Area() * mParams.TriangleCost( spec.NumRef() - i );
				float tieBreak = sqr( ( float ) i ) + sqr( ( float ) spec.NumRef() );
				if ( sah < split.SAH || ( sah == split.SAH && tieBreak < bestTieBreak ) )
				{
					split.SAH = sah;
					split.SortDim = mSortDim;
					split.NumLeft = i;
					split.LeftBounds = leftBounds;
					split.RightBounds = mRightBounds[i - 1];
					bestTieBreak = tieBreak;
				}
			}

		}


		return split;
	}

	void SplitBVHBuilder::PerformObjectSplit( NodeSpecification & left, NodeSpecification & right, const NodeSpecification & spec, const ObjectSplit & split )
	{
		mSortDim = split.SortDim;

		std::qsort( &mReferenceStack[mReferenceStack.size() - spec.NumRef()], spec.NumRef(), sizeof( Reference ), SplitBVHBuilder::SortCompare );

		left.Bounds		= split.LeftBounds;
		left.NumRef()	= split.NumLeft;
		right.Bounds	= split.RightBounds;
		right.NumRef()	= spec.NumRef() - split.NumLeft;
	}

	SplitBVHBuilder::SpatialSplit SplitBVHBuilder::FindSpatialSplit( const NodeSpecification & spec, float nodeSAH )
	{
		// Initialize bins
		float3 origin	= spec.Bounds.Min();
		float3 binSize	= ( spec.Bounds.Max() - origin ) * ( 1.0f / static_cast< float >( NumSpatialBins ) );
		float3 invBinSize = 1.0f / binSize;

		// Clean previous results
		for ( int32 dim = 0; dim < 3; dim++ )
		{
			for ( int32 i = 0; i < NumSpatialBins; i++ )
			{
				SpatialBin& bin = mBins[dim][i];
				bin.Bounds		= AABB();
				bin.enter		= 0;
				bin.exit		= 0;
			}
		}

		// Chop reference into bins

		for ( size_t refIdx = mReferenceStack.size() - spec.NumRef(); refIdx < mReferenceStack.size(); refIdx++ )
		{
			const Reference& ref = mReferenceStack[refIdx];
			int3 firstBin		 = clamp(int3(( ref.Bounds.Min() - origin ) * invBinSize), int3(0),  int3(NumSpatialBins - 1));
			int3 lastBin		 = clamp(int3(( ref.Bounds.Max() - origin ) * invBinSize), firstBin, int3(NumSpatialBins - 1));

			for ( int32 dim = 0; dim < 3; dim++ )
			{
				Reference currRef = ref;
				for ( int32 i = firstBin[dim]; i < lastBin[dim]; i++ )
				{
					Reference leftRef, rightRef;
					SplitReference( leftRef, rightRef, currRef, dim, origin[dim] + binSize[dim] * ( float ) ( i + 1 ) );
					mBins[dim][i].Bounds.Grow( leftRef.Bounds );
					currRef = rightRef;
				}
				mBins[dim][lastBin[dim]].Bounds.Grow( currRef.Bounds );
				mBins[dim][firstBin[dim]].enter++;
				mBins[dim][lastBin[dim]].exit--;
			}
		}

		// Select best split plane

		SpatialSplit split;
		for ( int32 dim = 0; dim < 3; dim++ )
		{
			//Sweep right to left and determine bounds

			AABB rightBounds;
			for ( int32 i = NumSpatialBins - 1; i > 0; i++ )
			{
				rightBounds.Grow( mBins[dim][i].Bounds );
				mRightBounds[i - 1] = rightBounds;
			}

			// Sweep left to right and select lowest SAH

			AABB leftBounds;
			int leftNum = 0;
			int rightNum = spec.NumRef();

			for ( int32 i = 1; i < NumSpatialBins; i++ )
			{
				leftBounds.Grow(mBins[dim][i-1].Bounds);
				leftNum	 += mBins[dim][i - 1].enter;
				rightNum -= mBins[dim][i - 1].exit;

				float sah = nodeSAH + leftBounds.Area() * mParams.TriangleCost( leftNum ) + mRightBounds[i - 1].Area() * mParams.TriangleCost( rightNum );
				if ( sah < split.SAH )
				{
					split.SAH = sah;
					split.Dim = dim;
					split.Pos = origin[dim] + binSize[dim] * ( float ) i;
				}
			}
		}

		return split;
	}

	void SplitBVHBuilder::PerformSpatialSplit( NodeSpecification & left, NodeSpecification & right, const NodeSpecification & spec, const SpatialSplit & split )
	{
		// Categorize references and compute bounds
		//
		// Left-hand size:		[leftStart, leftEnd[
		// Uncategorized/split:	[leftEnd, rightStart[
		// Right-hand side:		[rightStart, refs.getSize()[

		std::vector<Reference>& refs = mReferenceStack;
		int32 leftStart = (int32)refs.size() - spec.NumRef();
		int32 leftEnd	= leftStart;
		int32 rightStart = (int32)refs.size();
		left.Bounds = right.Bounds = AABB();

		for ( int32 i = leftEnd; i < rightStart; i++ )
		{
			// Entirely on the left-hand side?
			if ( refs[i].Bounds.Max()[split.Dim] <= split.Pos )
			{
				left.Bounds.Grow( refs[i].Bounds );
				Swap( refs[i], refs[leftEnd++] );
			}

			// Entirely on the right-hand size?
			else if ( refs[i].Bounds.Min()[split.Dim] >= split.Pos )
			{
				right.Bounds.Grow( refs[i].Bounds );
				Swap( refs[i--], refs[--rightStart] );
			}
		}

		// Duplicates or unsplit references intersecting both sides
		while ( leftEnd < rightStart)
		{
			// Split reference
			Reference lref, rref;
			SplitReference( lref, rref, refs[leftEnd], split.Dim, split.Pos );

			// Compute SAH for duplicate/unsplit candidates
			AABB lub = left.Bounds;  // Unsplit to left:     new left-hand bounds.
			AABB rub = right.Bounds; // Unsplit to right:    new right-hand bounds.
			AABB ldb = left.Bounds;  // Duplicate:           new left-hand bounds.
			AABB rdb = right.Bounds; // Duplicate:           new right-hand bounds.
			lub.Grow( refs[leftEnd].Bounds );
			rub.Grow( refs[leftEnd].Bounds );
			ldb.Grow( lref.Bounds );
			rdb.Grow( rref.Bounds );

			float lac = mParams.TriangleCost( leftEnd - leftStart );
			float rac = mParams.TriangleCost( (int32)refs.size() - rightStart );
			float lbc = mParams.TriangleCost( leftEnd - leftStart - 1 );
			float rbc = mParams.TriangleCost( (int32)refs.size() - rightStart + 1 );

			float unsplitLeftSAH  = lub.Area() * lbc + right.Bounds.Area() * rac;
			float unsplitRightSAH = left.Bounds.Area() * lac + rub.Area() * rbc;
			float duplicateSAH = ldb.Area() * lbc + rdb.Area() * rbc;
			float minSAH = min( unsplitLeftSAH, unsplitRightSAH, duplicateSAH );

			// Unsplit to left?
			if ( minSAH == unsplitLeftSAH )
			{
				left.Bounds = lub;
				leftEnd++;
			}

			// Unsplit to right?
			else if ( minSAH == unsplitRightSAH )
			{
				right.Bounds = rub;
				Swap( refs[leftEnd], refs[--rightStart] );
			}

			else
			{
				left.Bounds = ldb;
				right.Bounds = rdb;
				refs[leftEnd++] = lref;
				refs.push_back( rref );
			}
		}

		left.NumRef() = leftEnd - leftStart;
		right.NumRef() = (int32)refs.size() - rightStart;

	}

	void SplitBVHBuilder::SplitReference( Reference & left, Reference & right, const Reference & ref, int32 dim, float pos )
	{
		// Initialize references
		left.TriIdx() = right.TriIdx() = ref.TriIdx();
		left.Bounds = right.Bounds = AABB();

		// Loop over vertices/edges

		const int4*		tris	= mBVH.GetScene().TrianglesIndexesPtr();
		const float4*	verts	= mBVH.GetScene().VerticesPositionPtr();
		const int4&		inds	= tris[ref.TriIdx()];
		const float4*	v1		= &verts[inds.z];

		for ( int32 i = 0; i < 3; i++ )
		{
			const float4* v0 = v1;
			v1 = &verts[inds[i]];
			float v0p = v0->s[dim];
			float v1p = v1->s[dim];

			// Insert vertex to the box it belongs to

			if ( v0p <= pos )
				left.Bounds.Grow( *v0 );
			if ( v0p >= pos )
				right.Bounds.Grow( *v0 );

			// Edge intersects the plane -> insert intersection to both boxes.
			if ( (v0p < pos && v1p > pos) || (v0p > pos && v1p < pos) )
			{
				float4 t = lerp(*v0, *v1, clamp((pos - v0p)/(v1p-v0p), 0.0f, 1.0f));
				left.Bounds.Grow( t );
				right.Bounds.Grow( t );
			}
		}

		// Intersect with original bounds
		left.Bounds.Max()[dim] = pos;
		right.Bounds.Min()[dim] = pos;
		left.Bounds.Intersect( ref.Bounds );
		right.Bounds.Intersect( ref.Bounds );
	}

	int SplitBVHBuilder::SortCompare( const void* A, const void* B )
	{
		int dim = sPointer->mSortDim;
		Reference* refA = ( Reference* ) A;
		Reference* refB = ( Reference* ) B;
		float ca = refA->Bounds.Min()[dim] + refA->Bounds.Max()[dim];
		float cb = refB->Bounds.Min()[dim] + refB->Bounds.Max()[dim];
		return (ca < cb || (ca == cb && refA->TriIdx() < refB->TriIdx()));
	}
}
