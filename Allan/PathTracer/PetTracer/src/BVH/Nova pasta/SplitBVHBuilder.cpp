#include "SplitBVHBuilder.h"

#include <algorithm>

template <typename T>
T SwapRemove( std::vector<T>& Vector, size_t idx )
{
	std::cout << "Removed " << idx << std::endl;

	T old = Vector[idx];

	std::swap( Vector.begin() + idx, Vector.end() - 1 );
	Vector.pop_back();

	return old;
};

namespace PetTracer
{
	SplitBVHBuilder*	SplitBVHBuilder::sPointer = NULL;

	SplitBVHBuilder::SplitBVHBuilder( BVH & bvh, BuildParams& params )
		: mBVH(bvh),
		  mParams(params),
		  mMinOverlap(0.0f),
		  mSortDim(-1)
	{
	}

	BVHNode* SplitBVHBuilder::Run()
	{
		// Initialize reference stack and determine root bounds.

		const int3* tris	= ( const int3* ) mBVH.GetScene().TrianglesIndexesPtr();
		const float3* verts = ( const float3* ) mBVH.GetScene().VerticesPositionPtr();

		NodeSpecification rootSpec;
		rootSpec.NumRef() = mBVH.GetScene().TriangleCount();
		mReferenceStack.resize( rootSpec.NumRef() );

		for ( int32 i = 0; i < rootSpec.NumRef(); i++ )
		{
			mReferenceStack[i].TriIdx() = i;
			for ( int32 j = 0; j < 3; j++ )
				mReferenceStack[i].Bounds.Grow( verts[tris[i][j]] );
			rootSpec.Bounds.Grow( mReferenceStack[i].Bounds );
		}

		// Initialize rest of the members.

		mMinOverlap = rootSpec.Bounds.Area() * mParams.SplitAlpha;
		mRightBounds.resize( max( rootSpec.NumRef(), ( int32 ) NumSpatialBins ) - 1 );
		mNumDuplicates = 0;
		mTimer.Start();

		// Build recursively.

		BVHNode* root = BuildNode( rootSpec, 0, 0.0f, 1.0f );
		mBVH.TriangleIndices().shrink_to_fit();

		// Done.

		if ( mParams.EnablePrints )
			printf( "SplitBVHBuilder: progress %.0f%%, duplicates %.0f%%\n",
				100.0f, ( float ) mNumDuplicates / ( float ) mBVH.GetScene().TriangleCount() * 100.0f );
		return root;
	}

	int SplitBVHBuilder::SortCompare( const void* A, const void* B )
	{
		int dim = sPointer->mSortDim;
		Reference* refA = ( Reference* ) A;
		Reference* refB = ( Reference* ) B;
		float ca = refA->Bounds.Min()[dim] + refA->Bounds.Max()[dim];
		float cb = refB->Bounds.Min()[dim] + refB->Bounds.Max()[dim];
		return ( ca < cb || ( ca == cb && refA->TriIdx() < refB->TriIdx() ) );
	}

	bool SplitBVHBuilder::SortCompareV( Reference const& refA, Reference const& refB )
	{
		int32 dim = sPointer->mSortDim;
		float ca = refA.Bounds.Min()[dim] + refA.Bounds.Max()[dim];
		float cb = refB.Bounds.Min()[dim] + refB.Bounds.Max()[dim];
		return ( ca < cb || ( ca == cb && refA.TriIdx() < refB.TriIdx() ) );
	}


	BVHNode * SplitBVHBuilder::BuildNode( NodeSpecification spec, int level, float progressStart, float progressEnd )
	{
		// Display progress.

		if ( mParams.EnablePrints && mTimer.ElapsedTime() >= 1000.0f )
		{
			printf( "SplitBVHBuilder: progress %.0f%%, duplicates %.0f%%\r",
				progressStart * 100.0f, ( float ) mNumDuplicates / ( float ) mBVH.GetScene().TriangleCount() * 100.0f );
			mTimer.Start();
		}

		mBVH.NodeCount()++;

		// Remove degenerates.
		{
			int firstRef = mReferenceStack.size() - spec.NumRef();
			for ( int i = mReferenceStack.size() - 1; i >= firstRef; i-- )
			{
				float3 size = mReferenceStack[i].Bounds.Max() - mReferenceStack[i].Bounds.Min();
				if ( min( size ) < 0.0f || sum( size ) == max( size ) )
					SwapRemove( mReferenceStack, i );
					//mReferenceStack.removeSwap( i );
			}
			spec.numRef = mReferenceStack.size() - firstRef;
		}

		// Small enough or too deep => create leaf.

		if ( spec.numRef <= mParams.MinLeafSize || level >= MaxDepth )
			return CreateLeaf( spec );

		// Find split candidates.

		float area = spec.Bounds.Area();
		float leafSAH = area * mParams.TriangleCost( spec.numRef );
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

		// Leaf SAH is the lowest => create leaf.

		float minSAH = min( leafSAH, object.SAH, spatial.SAH );
		if ( minSAH == leafSAH && spec.numRef <= mParams.MaxLeafSize )
			return CreateLeaf( spec );

		// Perform split.

		NodeSpecification left, right;
		if ( minSAH == spatial.SAH )
			PerformSpatialSplit( left, right, spec, spatial );
		if ( !left.numRef || !right.numRef )
			PerformObjectSplit( left, right, spec, object );

		// Create inner node.

		mNumDuplicates += left.numRef + right.numRef - spec.numRef;
		float progressMid = lerp( progressStart, progressEnd, ( float ) right.numRef / ( float ) ( left.numRef + right.numRef ) );
		BVHNode* rightNode = BuildNode( right, level + 1, progressStart, progressMid );
		BVHNode* leftNode = BuildNode( left, level + 1, progressMid, progressEnd );
		return new InnerNode( spec.Bounds, leftNode, rightNode );
	}

	BVHNode * SplitBVHBuilder::CreateLeaf( NodeSpecification & spec )
	{
		std::vector<int32>& tris = mBVH.TriangleIndices();
		int32 triIdx = mReferenceStack[mReferenceStack.size() - 1].TriIdx();
		tris.push_back( triIdx );
		mReferenceStack.pop_back();
		return new LeafNode( spec.Bounds, triIdx, 1 );
	}

	SplitBVHBuilder::ObjectSplit SplitBVHBuilder::FindObjectSplit( const NodeSpecification & spec, float nodeSAH )
	{
		ObjectSplit split;
		const Reference* refPtr = &mReferenceStack[mReferenceStack.size() - spec.numRef];
		float bestTieBreak = FLT_MAX;

		// Sort along each dimension.

		for ( mSortDim = 0; mSortDim < 3; mSortDim++ )
		{
			//sort( this, m_refStack.getSize() - spec.numRef, m_refStack.getSize(), sortCompare, sortSwap );
			sPointer = this;
			std::sort( mReferenceStack.end() - spec.numRef, mReferenceStack.end(), SplitBVHBuilder::SortCompareV );

			// Sweep right to left and determine bounds.

			AABB rightBounds;
			for ( int32 i = spec.numRef - 1; i > 0; i-- )
			{
				rightBounds.Grow( refPtr[i].Bounds );
				mRightBounds[i - 1] = rightBounds;
			}

			// Sweep left to right and select lowest SAH.

			AABB leftBounds;
			for ( int32 i = 1; i < spec.numRef; i++ )
			{
				leftBounds.Grow( refPtr[i - 1].Bounds );
				float sah = nodeSAH + leftBounds.Area() * mParams.TriangleCost( i ) + mRightBounds[i - 1].Area() * mParams.TriangleCost( spec.numRef - i );
				float tieBreak = sqr( ( float ) i ) + sqr( ( float ) ( spec.numRef - i ) );
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
		std::sort( mReferenceStack.end() - spec.numRef, mReferenceStack.end(), SplitBVHBuilder::SortCompareV );

		left.numRef = split.NumLeft;
		left.Bounds = split.LeftBounds;
		right.numRef = spec.numRef - split.NumLeft;
		right.Bounds = split.RightBounds;
	}

	SplitBVHBuilder::SpatialSplit SplitBVHBuilder::FindSpatialSplit( const NodeSpecification & spec, float nodeSAH )
	{
		/*// Initialize bins
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
					SplitReference( leftRef, rightRef, currRef, dim, origin[dim] + binSize[dim] * (( float ) ( i + 1 )) );
					mBins[dim][i].Bounds.Grow( leftRef.Bounds );
					currRef = rightRef;
				}
				mBins[dim][lastBin[dim]].Bounds.Grow( currRef.Bounds );
				mBins[dim][firstBin[dim]].enter++;
				mBins[dim][lastBin[dim]].exit++;
			}
		}

		// Select best split plane

		SpatialSplit split;
		for ( int32 dim = 0; dim < 3; dim++ )
		{
			//Sweep right to left and determine bounds

			AABB rightBounds;
			for ( int32 i = NumSpatialBins - 1; i > 0; i-- )
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
				leftBounds.Grow(mBins[dim][i - 1].Bounds);
				leftNum	 += mBins[dim][i - 1].enter;
				rightNum -= mBins[dim][i - 1].exit;

				float sah = nodeSAH + leftBounds.Area() * mParams.TriangleCost( leftNum ) + mRightBounds[i - 1].Area() * mParams.TriangleCost( rightNum );
				if ( sah < split.SAH )
				{
					split.SAH = sah;
					split.Dim = dim;
					split.Pos = origin[dim] + (binSize[dim] * (( float ) i));
				}
			}
		}

		return split;*/

		// Initialize bins.

		float3 origin = spec.Bounds.Min();
		float3 binSize = ( spec.Bounds.Max() - origin ) * ( 1.0f / ( float ) NumSpatialBins );
		float3 invBinSize = 1.0f / binSize;

		for ( int dim = 0; dim < 3; dim++ )
		{
			for ( int i = 0; i < NumSpatialBins; i++ )
			{
				SpatialBin& bin = mBins[dim][i];
				bin.Bounds = AABB();
				bin.enter = 0;
				bin.exit = 0;
			}
		}

		// Chop references into bins.

		for ( int refIdx = mReferenceStack.size() - spec.numRef; refIdx < mReferenceStack.size(); refIdx++ )
		{
			const Reference& ref = mReferenceStack[refIdx];
			int3 firstBin = clamp( int3( ( ref.Bounds.Min() - origin ) * invBinSize ), 0, NumSpatialBins - 1 );
			int3 lastBin = clamp( int3( ( ref.Bounds.Max() - origin ) * invBinSize ), firstBin, NumSpatialBins - 1 );

			for ( int dim = 0; dim < 3; dim++ )
			{
				Reference currRef = ref;
				for ( int i = firstBin[dim]; i < lastBin[dim]; i++ )
				{
					Reference leftRef, rightRef;
					SplitReference( leftRef, rightRef, currRef, dim, origin[dim] + binSize[dim] * ( float ) ( i + 1 ) );
					mBins[dim][i].Bounds.Grow( leftRef.Bounds );
					currRef = rightRef;
				}
				mBins[dim][lastBin[dim]].Bounds.Grow( currRef.Bounds );
				mBins[dim][firstBin[dim]].enter++;
				mBins[dim][lastBin[dim]].exit++;
			}
		}

		// Select best split plane.

		SpatialSplit split;
		for ( int dim = 0; dim < 3; dim++ )
		{
			// Sweep right to left and determine bounds.

			AABB rightBounds;
			for ( int i = NumSpatialBins - 1; i > 0; i-- )
			{
				rightBounds.Grow( mBins[dim][i].Bounds );
				mRightBounds[i - 1] = rightBounds;
			}

			// Sweep left to right and select lowest SAH.

			AABB leftBounds;
			int leftNum = 0;
			int rightNum = spec.numRef;

			for ( int i = 1; i < NumSpatialBins; i++ )
			{
				leftBounds.Grow( mBins[dim][i - 1].Bounds );
				leftNum += mBins[dim][i - 1].enter;
				rightNum -= mBins[dim][i - 1].exit;

				float sah = nodeSAH + leftBounds.Area() * mParams.TriangleCost( leftNum ) + mRightBounds[i - 1].Area() * mParams.TriangleCost( rightNum );
				if ( sah < split.SAH )
				{
					split.SAH = sah;
					split.SAH = dim;
					split.SAH = origin[dim] + binSize[dim] * ( float ) i;
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
		int32 leftStart = refs.size() - spec.NumRef();
		int32 leftEnd = leftStart;
		int32 rightStart = refs.size();
		left.Bounds = AABB();
		right.Bounds = AABB();

		for ( int32 i = leftEnd; i < rightStart; i++ )
		{
			// Entirely on the left-hand side?

			if ( refs[i].Bounds.Max()[split.Dim] <= split.Pos )
			{
				left.Bounds.Grow( refs[i].Bounds );
				Swap( refs[i], refs[leftEnd++] );
			}

			// Entirely on the right-hand side?

			else if ( refs[i].Bounds.Min()[split.Dim] >= split.Pos )
			{
				right.Bounds.Grow( refs[i].Bounds );
				Swap( refs[i--], refs[--rightStart] );
			}
		}

		// Duplicates or unsplit references intersecting both sides
		while ( leftEnd < rightStart)
		{
			// Split reference.

			Reference lref, rref;
			SplitReference( lref, rref, refs[leftEnd], split.Dim, split.Pos );

			// Compute SAH for duplicate/unsplit candidates.

			AABB lub = left.Bounds;  // Unsplit to left:     new left-hand bounds.
			AABB rub = right.Bounds; // Unsplit to right:    new right-hand bounds.
			AABB ldb = left.Bounds;  // Duplicate:           new left-hand bounds.
			AABB rdb = right.Bounds; // Duplicate:           new right-hand bounds.
			lub.Grow( refs[leftEnd].Bounds );
			rub.Grow( refs[leftEnd].Bounds );
			ldb.Grow( lref.Bounds );
			rdb.Grow( rref.Bounds );

			float lac = mParams.TriangleCost( leftEnd - leftStart );
			float rac = mParams.TriangleCost( refs.size() - rightStart );
			float lbc = mParams.TriangleCost( leftEnd - leftStart + 1 );
			float rbc = mParams.TriangleCost( refs.size() - rightStart + 1 );


			float unsplitLeftSAH = lub.Area() * lbc + right.Bounds.Area() * rac;
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

			// Duplicate?

			else
			{
				left.Bounds = ldb;
				right.Bounds = rdb;
				refs[leftEnd++] = lref;
				refs.push_back( rref );
			}
		}

		left.NumRef() = leftEnd - leftStart;
		right.NumRef() = refs.size() - rightStart;

	}

	void SplitBVHBuilder::SplitReference( Reference& left, Reference& right, const Reference& ref, int32 dim, float pos )
	{
		// Initialize references.

		left.TriIdx() = ref.TriIdx();
		right.TriIdx() = ref.TriIdx();
		left.Bounds = AABB();
		right.Bounds = AABB();

		// Loop over vertices/edges.

		const int4* tris = ( const int4* ) mBVH.GetScene().TrianglesIndexesPtr();
		const float3* verts = ( const float3* ) mBVH.GetScene().VerticesPositionPtr();
		const int3& inds = tris[ref.TriIdx()];
		const float3* v1 = &verts[inds.z];

		for ( int i = 0; i < 3; i++ )
		{
			const float3* v0 = v1;
			v1 = &verts[inds[i]];
			float v0p = v0->s[ dim ];
			float v1p = v1->s[ dim ];

			// Insert vertex to the boxes it belongs to.

			if ( v0p <= pos )
				left.Bounds.Grow( *v0 );
			if ( v0p >= pos )
				right.Bounds.Grow( *v0 );

			// Edge intersects the plane => insert intersection to both boxes.

			if ( ( v0p < pos && v1p > pos ) || ( v0p > pos && v1p < pos ) )
			{
				float3 t = lerp( *v0, *v1, clamp( ( pos - v0p ) / ( v1p - v0p ), 0.0f, 1.0f ) );
				left.Bounds.Grow( t );
				right.Bounds.Grow( t );
			}
		}

		// Intersect with original bounds.

		left.Bounds.Max()[dim] = pos;
		right.Bounds.Min()[dim] = pos;
		left.Bounds.Intersect( ref.Bounds );
		right.Bounds.Intersect( ref.Bounds );
	}
}
