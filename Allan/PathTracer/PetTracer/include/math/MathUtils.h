#pragma once

#ifndef MATH_UTILS_H
#define MATH_UTILS_H

#ifdef _WIN32
#define NOMINMAX
#undef min
#undef max
#endif

#include <cmath>

#include "float2.h"
#include "float3.h"
#include "matrix.h"
#include "quaternion.h"


#define ONE_OVER_PI 0.31830988618f
#define PI			3.14159265358f
#define PI2			6.28318530718f
#define PI_OVER2	1.57079632679f
#define PI_OVER3	1.0471975512f
#define PI_OVER4	0.78539816339f

namespace PetTracer
{
	typedef	uint8_t		uint8;
	typedef	uint16_t	uint16;
	typedef uint32_t	uint32;
	typedef uint64_t	uint64;
	typedef int8_t		int8;
	typedef	int16_t		int16;
	typedef int32_t		int32;
	typedef int64_t		int64;

	/*template<typename T>
	inline T   min( T const&  x, T const&  y )  { return ( x < y ) ? x : y; }

	template<typename T>
	inline T   min( T const&  x, T const&  y, T const& z ) { return min( x, min( y, z ) ); }

	template<typename T>
	inline T   max( T const&  x, T const&  y )	{ return ( x > y ) ? x : y; }

	template<typename T>
	inline T   max( T const&  x, T const&  y, T const& z ) { return max( x, max( y, z ) ); }*/

	// FLOAT
	inline float min( float x, float y )
	{
		return ( x < y ) ? x : y;
	}

	inline float max( float x, float y )
	{
		return ( x > y ) ? x : y;
	}

	inline float min( float x, float y, float z )
	{
		return min( x, min( y, z ) );
	}

	inline float max( float x, float y, float z )
	{
		return max( x, max( y, z ) );
	}

	// INT
	inline int32 min( int32 x, int32 y )
	{
		return ( x < y ) ? x : y;
	}

	inline int32 max( int32 x, int32 y )
	{
		return ( x > y ) ? x : y;
	}

	inline int32 min( int32 x, int32 y, int32 z )
	{
		return min( x, min( y, z ) );
	}

	inline int32 max( int32 x, int32 y, int32 z )
	{
		return max( x, max( y, z ) );
	}

	inline int64 min( int64 x, int64 y )
	{
		return ( x < y ) ? x : y;
	}

	inline int64 max( int64 x, int64 y )
	{
		return ( x > y ) ? x : y;
	}

	inline quaternion rotation_quaternion( float3 const& axe, float angle )
	{
		// create (sin(a/2)*axis, cos(a/2)) quaternion
		// which rotates the point a radians around "axis"
		quaternion res;
		float3 u = axe; u.normalize();
		float sina2 = std::sin( angle / 2 );
		float cosa2 = std::cos( angle / 2 );

		res.x = sina2 * u.x;
		res.y = sina2 * u.y;
		res.z = sina2 * u.z;
		res.w = cosa2;

		return res;
	}

	template<typename T>
	inline float sum( vec3<T> const& f )
	{
		return f.x + f.y + f.z;
	}

	template<typename T>
	inline T max( vec3<T> const& f )
	{
		return max( f.x, f.y, f.z );
	}

	template<typename T>
	inline T min( vec3<T> const& f )
	{
		return min( f.x, f.y, f.z );
	}

	inline float3 max( float3 const& f1, float3 const& f2 )
	{
		return float3( max( f1.x, f2.x ), max( f1.y, f2.y ), max( f1.z, f2.z ) );
	}

	template<typename T>
	inline vec3<T> max( vec3<T> const& f1, vec3<T> const& f2 )
	{
		return vec3<T>( max( f1.x, f2.x ), max( f1.y, f2.y ), max( f1.z, f2.z ), max( f1.w, f2.w ));
	};

	template<typename T>
	inline vec3<T> max( vec3<T> const& f1, vec3<T> const& f2, vec3<T> const& f3 )
	{
		return vec3<T>( max( f1.x, f2.x, f3.x), max( f1.y, f2.y, f3.y ), max( f1.z, f2.z, f3.z ), max( f1.w, f2.w, f3.w ) );
	};

	inline float3 min( float3 const& f1, float3 const& f2 )
	{
		return float3( min( f1.x, f2.x ), min( f1.y, f2.y ), min( f1.z, f2.z ) );
	}

	template<typename T>
	inline vec3<T> min( vec3<T> const& f1, vec3<T> const& f2 )
	{
		return vec3<T>( min( f1.x, f2.x ), min( f1.y, f2.y ), min( f1.z, f2.z ), min( f1.w, f2.w ) );
	};

	template<typename T>
	inline vec3<T> min( vec3<T> const& f1, vec3<T> const& f2, vec3<T> const& f3 )
	{
		return vec3<T>( min( f1.x, f2.x, f3.x ), min( f1.y, f2.y, f3.y ), min( f1.z, f2.z, f3.z ), min( f1.w, f2.w, f3.w ) );
	};

	inline float sqr( float x ) { return x*x; };
	
	inline int3 clamp( int3 const& v1, int3 const& pmin, int3 const& pmax )
	{
		return min( pmax, max( pmin, v1 ) );
	}

	template<typename T>
	inline T clamp( T const& v1, T const& pmin, T const& pmax )
	{
		return min( pmax, max( pmin, v1 ) );
	}

	template<typename T, typename N>
	inline T lerp( T const& v1, T const& v2, N const& u )
	{
		return (1 - u) * v1 + (u) * v2;
	}
	
	/*template<typename T>
	inline vec3<T> clamp( vec3<T> const& v1, vec3<T> const& nMin, vec3<T> const& nMax )
	{
		return min( nMax, max( nMin, v1));
	}*/

	template <typename T>
	inline void Swap( T& a, T& b ) { T t = a; a = b; b = t; }

	class AABB
	{
	public:
		AABB( float4 const& pMin, float4 const& pMax )
		{
			mMin = pMin;
			mMax = pMax;
		}

		AABB() : mMin( FLT_MAX, FLT_MAX, FLT_MAX ), mMax( -FLT_MAX, -FLT_MAX, -FLT_MAX )
		{
		}

		bool inline Valid() const
		{
			return (mMin.x <= mMax.x) && (mMin.y <= mMax.y) && (mMin.z <= mMax.z);
		}

		inline float Area() const
		{
			if ( !Valid() ) return 0.0f;
			float4 size = mMax - mMin;
			return 2.0f*( size.x * size.y
				+ size.x * size.z
				+ size.y * size.z );
		}

		inline float4& Min() { return mMin; };
		inline float4& Max() { return mMax; };

		inline float4 Min() const { return mMin; };
		inline float4 Max() const { return mMax; };

		inline void Grow( float3 const& vec )
		{
			mMin = min( mMin, vec );
			mMax = max( mMax, vec );
		}

		inline void Grow( AABB const& aabb )
		{
			Grow( aabb.Min() );
			Grow( aabb.Max() );
		}

		inline void Intersect( AABB const& aabb )
		{
			mMin = max( mMin, aabb.mMin );
			mMax = min( mMax, aabb.mMax );
		}

		inline AABB& operator=( AABB const& rhs ) { mMin = rhs.mMin; mMax = rhs.mMax; return *this; }
		inline AABB( AABB const& rhs ) { mMin = rhs.mMin; mMax = rhs.mMax; }

	private:
		float4 mMin;
		float4 mMax;
	};
}

#endif // MATH_UTILS_H