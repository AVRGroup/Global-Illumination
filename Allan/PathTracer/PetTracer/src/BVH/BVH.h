#pragma once

#include <math/MathUtils.h>

namespace PetTracer
{
	class AABB
	{
	public:
		AABB( float4 const& pMin, float4 const& pMax )
		{
			mMin = min( pMin, pMax );
			mMax = max( pMin, pMax );
		}

		AABB( ) : mMin(), mMax()
		{
		}

		inline float Area() const
		{
			float4 size = mMax - mMin;
			return 2.0f*( size.x * size.y
				        + size.x * size.z
				        + size.y * size.z );
		}

		inline float4 Min() const { return mMin; };
		inline float4 Max() const { return mMax; };

		inline void Grow(float4 const& vec)
		{
			mMin = min( mMin, vec );
			mMax = max( mMax, vec );
		}

		inline void Grow(AABB const& aabb)
		{
			mMin = min( mMin, aabb.mMin );
			mMax = max( mMax, aabb.mMax );
		}

	private:
		float4 mMin;
		float4 mMax;
	};

	class BVHNode
	{
	public:
		BVHNode( AABB const& aabb ) : mBounds( aabb ) { }

		AABB Bounds() const { return mBounds; };
		AABB& Bounds() { return mBounds; };

		inline float Area() const { return mBounds.Area(); };

	private:
		AABB mBounds;
	};

	/** BVH Tree
	 *
	 */
	class BVH
	{

	};
}