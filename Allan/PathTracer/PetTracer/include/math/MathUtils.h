#pragma once

#ifndef MATH_UTILS_H
#define MATH_UTILS_H

#ifdef _WIN32
#define NOMINMAX
#undef min
#undef max
#endif

#include <CL/cl.h>
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

	inline int8   min( int8  x, int8  y )   { return ( x < y ) ? x : y; }
	inline int16  min( int16 x, int16 y )   { return ( x < y ) ? x : y; }
	inline int32  min( int32 x, int32 y )   { return ( x < y ) ? x : y; }
	inline int64  min( int64 x, int64 y )   { return ( x < y ) ? x : y; }
	inline uint8  min( uint8  x, uint8  y ) { return ( x < y ) ? x : y; }
	inline uint16 min( uint16 x, uint16 y ) { return ( x < y ) ? x : y; }
	inline uint32 min( uint32 x, uint32 y ) { return ( x < y ) ? x : y; }
	inline uint64 min( uint64 x, uint64 y ) { return ( x < y ) ? x : y; }

	inline int8   max( int8  x, int8  y )   { return ( x > y ) ? x : y; }
	inline int16  max( int16 x, int16 y )   { return ( x > y ) ? x : y; }
	inline int32  max( int32 x, int32 y )   { return ( x > y ) ? x : y; }
	inline int64  max( int64 x, int64 y )   { return ( x > y ) ? x : y; }
	inline uint8  max( uint8  x, uint8  y ) { return ( x > y ) ? x : y; }
	inline uint16 max( uint16 x, uint16 y ) { return ( x > y ) ? x : y; }
	inline uint32 max( uint32 x, uint32 y ) { return ( x > y ) ? x : y; }
	inline uint64 max( uint64 x, uint64 y ) { return ( x > y ) ? x : y; }

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
	inline float max( vec3<T> const& f )
	{
		return max( f.x, max( f.y, f.z ) );
	}

	template<typename T>
	inline float min( vec3<T> const& f )
	{
		return min( f.x, min( f.y, f.z ) );
	}

	template<typename T>
	inline vec3<T> max( vec3<T> const& f1, vec3<T> const& f2 )
	{
		return vec3<T>( max( f1.x, f2.x ), max( f1.y, f2.y ), max( f1.z, f2.z ), max( f1.w, f2.w ));
	};

	template<typename T>
	inline vec3<T> max( vec3<T> const& f1, vec3<T> const& f2, vec3<T> const& f3 )
	{
		return vec3<T>( max(max( f1.x, f2.x ), f3.x), max( max( f1.y, f2.y ), f3.y ), max( max( f1.z, f2.z ), f3.z ), max( max( f1.w, f2.w ), f3.w ) );
	};

	template<typename T>
	inline vec3<T> min( vec3<T> const& f1, vec3<T> const& f2 )
	{
		return vec3<T>( min( f1.x, f2.x ), min( f1.y, f2.y ), min( f1.z, f2.z ), min( f1.w, f2.w ) );
	};

	template<typename T>
	inline vec3<T> min( vec3<T> const& f1, vec3<T> const& f2, vec3<T> const& f3 )
	{
		return vec3<T>( min( min( f1.x, f2.x ), f3.x ), min( min( f1.y, f2.y ), f3.y ), min( min( f1.z, f2.z ), f3.z ), min( min( f1.w, f2.w ), f3.w ) );
	};

	inline float sqr( float x ) { return x*x; };
	
	template<typename T>
	inline T clamp( T const& v1, T const& pmin, T const& pmax )
	{
		return min( pmax, max( pmin, v1 ) );
	}

	template<typename T, typename N>
	inline T lerp( T const& v1, T const& v2, N const& u )
	{
		return u * v1 + ( 1 - u ) * v2;
	}
	
	/*template<typename T>
	inline vec3<T> clamp( vec3<T> const& v1, vec3<T> const& nMin, vec3<T> const& nMax )
	{
		return min( nMax, max( nMin, v1));
	}*/

	template <typename T>
	inline void Swap( T& a, T& b ) { T t = a; b = a; a = t; }

	class AABB
	{
	public:
		AABB( float4 const& pMin, float4 const& pMax )
		{
			mMin = min( pMin, pMax );
			mMax = max( pMin, pMax );
		}

		AABB() : mMin(), mMax()
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

		inline void Grow( float4 const& vec )
		{
			float t0 = mMin.w, t1 = mMax.w;
			mMin = min( mMin, vec );
			mMax = max( mMax, vec );
			mMin.w = t0;
			mMax.w = t1;
		}

		inline void Grow( AABB const& aabb )
		{
			float t0 = mMin.w, t1 = mMax.w;
			mMin = min( mMin, aabb.mMin );
			mMax = max( mMax, aabb.mMax );
			mMin.w = t0;
			mMax.w = t1;
		}

		inline void Intersect( AABB const& aabb )
		{
			float t0 = mMin.w, t1 = mMax.w;
			mMin = max( mMin, aabb.mMin );
			mMax = min( mMax, aabb.mMax );
			mMin.w = t0;
			mMax.w = t1;
		}

		// Cast mMin.w to signed int
		inline int32& Low() { return *( signed int* ) &mMin.w; };
		inline int32  Low() const { return *( signed int* ) &mMin.w; };

		// Cast mMax.w to signed int
		inline int32& High() { return *( signed int* ) &mMax.w; };
		inline int32  High() const { return *( signed int* ) &mMax.w; };

	private:
		float4 mMin;
		float4 mMax;
	};
}

#endif // !Math_UTILS_H