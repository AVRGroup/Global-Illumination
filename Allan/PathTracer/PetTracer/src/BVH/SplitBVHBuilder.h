/*
*  Copyright (c) 2009-2011, NVIDIA Corporation
*  All rights reserved.
*
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions are met:
*      * Redistributions of source code must retain the above copyright
*        notice, this list of conditions and the following disclaimer.
*      * Redistributions in binary form must reproduce the above copyright
*        notice, this list of conditions and the following disclaimer in the
*        documentation and/or other materials provided with the distribution.
*      * Neither the name of NVIDIA Corporation nor the
*        names of its contributors may be used to endorse or promote products
*        derived from this software without specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
*  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
*  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
*  DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
*  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
*  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
*  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
*  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
*  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
*  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

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
		BVH&                    m_bvh;
		const BuildParams&      m_params;

		std::vector<Reference>  m_refStack;
		float                     m_minOverlap;
		std::vector<AABB>       m_rightBounds;
		int32                     m_sortDim;
		SpatialBin              m_bins[3][NumSpatialBins];

		Timer<milliseconds>     m_progressTimer;
		int32                     m_numDuplicates;
	};

	//------------------------------------------------------------------------
}
