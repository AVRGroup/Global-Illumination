#include "SplitBVHBuilder.h"

using namespace PetTracer;

SplitBVHBuilder* SplitBVHBuilder::sPointer = NULL;

//------------------------------------------------------------------------

SplitBVHBuilder::SplitBVHBuilder( BVH& bvh, const BuildParams& params )
	: mBVH( bvh ),
	mParams( params ),
	mMinOverlap( 0.0f ),
	mSortDim( -1 )
{
}

//------------------------------------------------------------------------

SplitBVHBuilder::~SplitBVHBuilder( void )
{
}

//------------------------------------------------------------------------

BVHNode* SplitBVHBuilder::Run( void )
{
	// Initialize reference stack and determine root bounds.

	const int3* tris = ( const int3* ) mBVH.GetScene().TrianglesIndexesPtr();
	const float4* verts = ( const float3* ) mBVH.GetScene().VerticesPositionPtr();

	NodeSpec rootSpec;
	rootSpec.numRef = mBVH.GetScene().TriangleCount();
	mReferenceStack.resize( rootSpec.numRef );

	for ( int32 i = 0; i < rootSpec.numRef; i++ )
	{
		mReferenceStack[i].triIdx = i;
		for ( int32 j = 0; j < 3; j++ )
			mReferenceStack[i].bounds.Grow( verts[tris[i][j]] );
		rootSpec.bounds.Grow( mReferenceStack[i].bounds );
	}

	// Initialize rest of the members.

	mMinOverlap = rootSpec.bounds.Area() * mParams.SplitAlpha;
	mRightBounds.clear();
	mRightBounds.resize( max( rootSpec.numRef, ( int32 ) NumSpatialBins ) - 1 );
	mNumDuplicates = 0;
	mProgressTimer.Start();

	// Build recursively.
	sPointer = this;
	BVHNode* root = buildNode( rootSpec, 0, 0.0f, 1.0f );
	mBVH.TriangleIndices().shrink_to_fit();

	// Done.

	if ( mParams.EnablePrints )
		printf( "SplitBVHBuilder: progress %.0f%%, duplicates %.0f%%\n",
			100.0f, ( float ) mNumDuplicates / ( float ) mBVH.GetScene().TriangleCount() * 100.0f );
	return root;
}

//------------------------------------------------------------------------

int SplitBVHBuilder::sortCompare( const void* a, const void* b )
{
	int32 dim = sPointer->mSortDim;
	const Reference& ra = *((Reference*)a);
	const Reference& rb = *((Reference*)b);
	float ca = ra.bounds.Min()[dim] + ra.bounds.Max()[dim];
	float cb = rb.bounds.Min()[dim] + rb.bounds.Max()[dim];
	return ( ca < cb || ( ca == cb && ra.triIdx < rb.triIdx ) ) ? 1 : -1;
}

//------------------------------------------------------------------------

void SplitBVHBuilder::sortSwap( void* data, int32 idxA, int32 idxB )
{
	SplitBVHBuilder* ptr = ( SplitBVHBuilder* ) data;
	Swap( ptr->mReferenceStack[idxA], ptr->mReferenceStack[idxB] );
}

//------------------------------------------------------------------------

BVHNode* SplitBVHBuilder::buildNode( NodeSpec spec, int32 level, float progressStart, float progressEnd )
{
	// Display progress.

	if ( mParams.EnablePrints && mProgressTimer.ElapsedTime() >= 1.0f )
	{
		printf( "SplitBVHBuilder: progress %.0f%%, duplicates %.0f%%\r",
			progressStart * 100.0f, ( float ) mNumDuplicates / ( float ) mBVH.GetScene().TriangleCount() * 100.0f );
		mProgressTimer.Start();
	}

	mBVH.NodeCount()++;

	// Remove degenerates.
	{
		int32 firstRef = static_cast<int32>(mReferenceStack.size() - spec.numRef);
		for ( uint64 i = mReferenceStack.size() - 1; i > firstRef; i-- )
		{
			float3 size = mReferenceStack[i].bounds.Max() - mReferenceStack[i].bounds.Min();
			if ( min( size ) < 0.0f || sum( size ) == max( size ) )
			{
				auto old = mReferenceStack[i];
				mReferenceStack[i] = mReferenceStack[mReferenceStack.size() - 1];
				mReferenceStack.pop_back();
			}
		}
		spec.numRef =  static_cast<int32>( mReferenceStack.size() - firstRef );
	}

	// Small enough or too deep => create leaf.

	if ( spec.numRef <= mParams.MinLeafSize || level >= MaxDepth )
		return createLeaf( spec );

	// Find split candidates.

	float area = spec.bounds.Area();
	float leafSAH = area * mParams.TriangleCost( spec.numRef );
	float nodeSAH = area * mParams.NodeCost( 2 );
	ObjectSplit object = findObjectSplit( spec, nodeSAH );

	SpatialSplit spatial;
	if ( level < MaxSpatialDepth )
	{
		AABB overlap = object.leftBounds;
		overlap.Intersect( object.rightBounds );
		if ( overlap.Area() >= mMinOverlap )
			spatial = findSpatialSplit( spec, nodeSAH );
	}

	// Leaf SAH is the lowest => create leaf.

	float minSAH = min( leafSAH, object.sah, spatial.sah );
	if ( minSAH == leafSAH && spec.numRef <= mParams.MaxLeafSize )
		return createLeaf( spec );

	// Perform split.

	NodeSpec left, right;
	if ( minSAH == spatial.sah )
		performSpatialSplit( left, right, spec, spatial );
	if ( !left.numRef || !right.numRef )
		performObjectSplit( left, right, spec, object );

	// Create inner node.

	mNumDuplicates += left.numRef + right.numRef - spec.numRef;
	float progressMid = lerp( progressStart, progressEnd, ( float ) right.numRef / ( float ) ( left.numRef + right.numRef ) );
	BVHNode* rightNode = buildNode( right, level + 1, progressStart, progressMid );
	BVHNode* leftNode = buildNode( left, level + 1, progressMid, progressEnd );
	return new InnerNode( spec.bounds, leftNode, rightNode );
}

//------------------------------------------------------------------------

BVHNode* SplitBVHBuilder::createLeaf( const NodeSpec& spec )
{
	std::vector<int32>& tris = mBVH.TriangleIndices();
	for ( int32 i = 0; i < spec.numRef; i++ )
	{
		tris.push_back( mReferenceStack[mReferenceStack.size()-1].triIdx );
		mReferenceStack.pop_back();
	}
	//return new LeafNode( spec.bounds, (int32)tris.size() - spec.numRef, (int32)tris.size() );
	return new LeafNode( spec.bounds, tris[tris.size() - spec.numRef], 1 );
}

//------------------------------------------------------------------------

SplitBVHBuilder::ObjectSplit SplitBVHBuilder::findObjectSplit( const NodeSpec& spec, float nodeSAH )
{
	ObjectSplit split;
	const Reference* refPtr = &mReferenceStack[ mReferenceStack.size() - spec.numRef ];
	float bestTieBreak = FLT_MAX;

	// Sort along each dimension.

	for ( mSortDim = 0; mSortDim < 3; mSortDim++ )
	{
		std::qsort( &mReferenceStack[mReferenceStack.size() - spec.numRef], spec.numRef, sizeof( SplitBVHBuilder::Reference ), sortCompare );
		//sort( this, mReferenceStack.size() - spec.numRef, mReferenceStack.size(), sortCompare, sortSwap );

		// Sweep right to left and determine bounds.

		AABB rightBounds;
		for ( int32 i = spec.numRef - 1; i > 0; i-- )
		{
			rightBounds.Grow( refPtr[i].bounds );
			mRightBounds[i - 1] = rightBounds;
		}

		// Sweep left to right and select lowest SAH.

		AABB leftBounds;
		for ( int32 i = 1; i < spec.numRef; i++ )
		{
			leftBounds.Grow( refPtr[i - 1].bounds );
			float sah = nodeSAH + leftBounds.Area() * mParams.TriangleCost( i ) + mRightBounds[i - 1].Area() * mParams.TriangleCost( spec.numRef - i );
			float tieBreak = sqr( ( float ) i ) + sqr( ( float ) ( spec.numRef - i ) );
			if ( sah < split.sah || ( sah == split.sah && tieBreak < bestTieBreak ) )
			{
				split.sah = sah;
				split.sortDim = mSortDim;
				split.numLeft = i;
				split.leftBounds = leftBounds;
				split.rightBounds = mRightBounds[i - 1];
				bestTieBreak = tieBreak;
			}
		}
	}
	return split;
}

//------------------------------------------------------------------------

void SplitBVHBuilder::performObjectSplit( NodeSpec& left, NodeSpec& right, const NodeSpec& spec, const ObjectSplit& split )
{
	mSortDim = split.sortDim;
	std::qsort( &mReferenceStack[mReferenceStack.size() - spec.numRef], spec.numRef, sizeof( SplitBVHBuilder::Reference ), sortCompare );
	//sort( this, mReferenceStack.size() - spec.numRef, mReferenceStack.size(), sortCompare, sortSwap );

	left.numRef = split.numLeft;
	left.bounds = split.leftBounds;
	right.numRef = spec.numRef - split.numLeft;
	right.bounds = split.rightBounds;
}

//------------------------------------------------------------------------

SplitBVHBuilder::SpatialSplit SplitBVHBuilder::findSpatialSplit( const NodeSpec& spec, float nodeSAH )
{
	// Initialize bins.

	float3 origin = spec.bounds.Min();
	float3 binSize = ( spec.bounds.Max() - origin ) * ( 1.0f / ( float ) NumSpatialBins );
	float3 invBinSize = 1.0f / binSize;

	for ( int32 dim = 0; dim < 3; dim++ )
	{
		for ( int32 i = 0; i < NumSpatialBins; i++ )
		{
			SpatialBin& bin = mBins[dim][i];
			bin.bounds = AABB();
			bin.enter = 0;
			bin.exit = 0;
		}
	}

	// Chop references into bins.

	for ( uint64 refIdx = mReferenceStack.size() - spec.numRef; refIdx < mReferenceStack.size(); refIdx++ )
	{
		const Reference& ref = mReferenceStack[refIdx];
		int3 firstBin = clamp( int3( ( ref.bounds.Min() - origin ) * invBinSize ), 0, NumSpatialBins - 1 );
		int3 lastBin = clamp( int3( ( ref.bounds.Max() - origin ) * invBinSize ), firstBin, NumSpatialBins - 1 );

		for ( int32 dim = 0; dim < 3; dim++ )
		{
			Reference currRef = ref;
			for ( int32 i = firstBin[dim]; i < lastBin[dim]; i++ )
			{
				Reference leftRef, rightRef;
				splitReference( leftRef, rightRef, currRef, dim, origin[dim] + binSize[dim] * ( float ) ( i + 1 ) );
				mBins[dim][i].bounds.Grow( leftRef.bounds );
				currRef = rightRef;
			}
			mBins[dim][lastBin[dim]].bounds.Grow( currRef.bounds );
			mBins[dim][firstBin[dim]].enter++;
			mBins[dim][lastBin[dim]].exit++;
		}
	}

	// Select best split plane.

	SpatialSplit split;
	for ( int32 dim = 0; dim < 3; dim++ )
	{
		// Sweep right to left and determine bounds.

		AABB rightBounds;
		for ( int32 i = NumSpatialBins - 1; i > 0; i-- )
		{
			rightBounds.Grow( mBins[dim][i].bounds );
			mRightBounds[i - 1] = rightBounds;
		}

		// Sweep left to right and select lowest SAH.

		AABB leftBounds;
		int32 leftNum = 0;
		int32 rightNum = spec.numRef;

		for ( int32 i = 1; i < NumSpatialBins; i++ )
		{
			leftBounds.Grow( mBins[dim][i - 1].bounds );
			leftNum += mBins[dim][i - 1].enter;
			rightNum -= mBins[dim][i - 1].exit;

			float sah = nodeSAH + leftBounds.Area() * mParams.TriangleCost( leftNum ) + mRightBounds[i - 1].Area() * mParams.TriangleCost( rightNum );
			if ( sah < split.sah )
			{
				split.sah = sah;
				split.dim = dim;
				split.pos = origin[dim] + binSize[dim] * ( float ) i;
			}
		}
	}
	return split;
}

//------------------------------------------------------------------------

void SplitBVHBuilder::performSpatialSplit( NodeSpec& left, NodeSpec& right, const NodeSpec& spec, const SpatialSplit& split )
{
	// Categorize references and compute bounds.
	//
	// Left-hand side:      [leftStart, leftEnd[
	// Uncategorized/split: [leftEnd, rightStart[
	// Right-hand side:     [rightStart, refs.size()[

	std::vector<Reference>& refs = mReferenceStack;
	int32 leftStart = (int32)refs.size() - spec.numRef;
	int32 leftEnd = leftStart;
	int32 rightStart = (int32)refs.size();
	left.bounds = right.bounds = AABB();

	for ( int32 i = leftEnd; i < rightStart; i++ )
	{
		// Entirely on the left-hand side?

		if ( refs[i].bounds.Max()[split.dim] <= split.pos )
		{
			left.bounds.Grow( refs[i].bounds );
			Swap( refs[i], refs[leftEnd++] );
		}

		// Entirely on the right-hand side?

		else if ( refs[i].bounds.Min()[split.dim] >= split.pos )
		{
			right.bounds.Grow( refs[i].bounds );
			Swap( refs[i--], refs[--rightStart] );
		}
	}

	// Duplicate or unsplit references intersecting both sides.

	while ( leftEnd < rightStart )
	{
		// Split reference.

		Reference lref, rref;
		splitReference( lref, rref, refs[leftEnd], split.dim, split.pos );

		// Compute SAH for duplicate/unsplit candidates.

		AABB lub = left.bounds;  // Unsplit to left:     new left-hand bounds.
		AABB rub = right.bounds; // Unsplit to right:    new right-hand bounds.
		AABB ldb = left.bounds;  // Duplicate:           new left-hand bounds.
		AABB rdb = right.bounds; // Duplicate:           new right-hand bounds.
		lub.Grow( refs[leftEnd].bounds );
		rub.Grow( refs[leftEnd].bounds );
		ldb.Grow( lref.bounds );
		rdb.Grow( rref.bounds );

		float lac = mParams.TriangleCost( leftEnd - leftStart );
		float rac = mParams.TriangleCost( (int32) refs.size() - rightStart );
		float lbc = mParams.TriangleCost( leftEnd - leftStart + 1 );
		float rbc = mParams.TriangleCost( (int32) refs.size() - rightStart + 1 );

		float unsplitLeftSAH = lub.Area() * lbc + right.bounds.Area() * rac;
		float unsplitRightSAH = left.bounds.Area() * lac + rub.Area() * rbc;
		float duplicateSAH = ldb.Area() * lbc + rdb.Area() * rbc;
		float minSAH = min( unsplitLeftSAH, unsplitRightSAH, duplicateSAH );

		// Unsplit to left?

		if ( minSAH == unsplitLeftSAH )
		{
			left.bounds = lub;
			leftEnd++;
		}

		// Unsplit to right?

		else if ( minSAH == unsplitRightSAH )
		{
			right.bounds = rub;
			Swap( refs[leftEnd], refs[--rightStart] );
		}

		// Duplicate?

		else
		{
			left.bounds = ldb;
			right.bounds = rdb;
			refs[leftEnd++] = lref;
			refs.push_back( rref );
		}
	}

	left.numRef = leftEnd - leftStart;
	right.numRef = (int32)refs.size() - rightStart;
}

//------------------------------------------------------------------------

void SplitBVHBuilder::splitReference( Reference& left, Reference& right, const Reference& ref, int32 dim, float pos )
{
	// Initialize references.

	left.triIdx = right.triIdx = ref.triIdx;
	left.bounds = right.bounds = AABB();

	// Loop over vertices/edges.

	const int3* tris = ( const int3* ) mBVH.GetScene().TrianglesIndexesPtr();
	const float3* verts = ( const float3* ) mBVH.GetScene().VerticesPositionPtr();
	const int3& inds = tris[ref.triIdx];
	const float3* v1 = &verts[inds.z];

	for ( int32 i = 0; i < 3; i++ )
	{
		const float3* v0 = v1;
		v1 = &verts[inds[i]];
		float v0p = v0->s[dim];
		float v1p = v1->s[dim];

		// Insert vertex to the boxes it belongs to.

		if ( v0p <= pos )
			left.bounds.Grow( *v0 );
		if ( v0p >= pos )
			right.bounds.Grow( *v0 );

		// Edge intersects the plane => insert intersection to both boxes.

		if ( ( v0p < pos && v1p > pos ) || ( v0p > pos && v1p < pos ) )
		{
			float3 t = lerp( *v0, *v1, clamp( ( pos - v0p ) / ( v1p - v0p ), 0.0f, 1.0f ) );
			left.bounds.Grow( t );
			right.bounds.Grow( t );
		}
	}

	// Intersect with original bounds.

	left.bounds.Max()[dim] = pos;
	right.bounds.Min()[dim] = pos;
	left.bounds.Intersect( ref.bounds );
	right.bounds.Intersect( ref.bounds );
}

//------------------------------------------------------------------------
