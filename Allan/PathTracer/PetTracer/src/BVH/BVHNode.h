#pragma once

#include "math/MathUtils.h"

namespace PetTracer
{
	class BVHNode
	{
	public:
		BVHNode( AABB const& aabb ) { mBounds = aabb; }

		AABB  Bounds() const { return mBounds; };
		AABB& Bounds() { return mBounds; };

		inline float Area() const { return mBounds.Area(); };

		inline void DeleteSubTree()
		{
			for ( uint32 i = 0; i < NumChildNodes(); i++ )
			{
				GetChildNode( i )->DeleteSubTree();
			}

			delete this;
		}

		virtual bool	 IsLeaf() const = 0;
		virtual uint32	 NumChildNodes() = 0;
		virtual BVHNode* GetChildNode( uint32 i ) = 0;
		virtual uint32	 NumTriangles() = 0;

	protected:
		AABB mBounds;
	};

	class InnerNode : public BVHNode
	{
	public:
		InnerNode( AABB const& aabb, BVHNode* child0, BVHNode* child1 ) : BVHNode( aabb ) { mChildren[0] = child0, mChildren[1] = child1; };

		inline bool		IsLeaf() const override { return false; };

		inline uint32	NumChildNodes() override { return 2; };

		inline BVHNode* GetChildNode( uint32 i ) override { return ( i >= 0 && i <= 2 ) ? mChildren[i] : NULL; };

		inline uint32	NumTriangles() override { return 0; };

		BVHNode* mChildren[2];
	};

	class LeafNode : public BVHNode
	{
	public:
		LeafNode( AABB const& aabb, int32 low, int32 high ) : BVHNode( aabb ) { Low() = low, High() = high; };

		inline bool		IsLeaf() const override { return true; };

		inline uint32	NumChildNodes() override { return 0; };

		inline BVHNode* GetChildNode( uint32 i ) override { return NULL; };

		inline uint32	NumTriangles() override { return High() - Low(); };



		inline int32& Low() { return mLow; };
		inline int32  Low() const { return mLow; };
		inline int32& High() { return mHigh; };
		inline int32  High() const { return mHigh; };

	private:
		int32 mLow;
		int32 mHigh;

	};
}