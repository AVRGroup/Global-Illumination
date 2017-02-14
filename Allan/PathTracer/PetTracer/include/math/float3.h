#pragma once

#ifndef FLOAT3_H
#define FLOAT3_H

#include <CL/cl.h>
#include <cmath>
#include <iostream>

namespace PetTracer
{
	template<typename T>
	class vec3
	{
	public:
		vec3(  ) : x( 0 ), y( 0 ), z( 0 ), w( 0 ) { };
		vec3( T _x, T _y, T _z, T _w = 0 ) : x( _x ), y( _y ), z( _z ), w( _w ) { };
		vec3( T _x ) : x( _x ), y( _x ), z( _x ), w( _x ) { };

		template<typename N>
		vec3( vec3<N> const& v ) { x = static_cast< T >( v.x ), y = static_cast< T >( v.y ), z = static_cast< T >( v.z ), w = static_cast< T >( v.w ); };

		inline T& operator[]( int i )		{ return s[i]; }
		inline T operator[]( int i ) const	{ return s[i]; }
		inline vec3 operator-() const			{ return vec3( -x, -y, -z, -w ); }

		inline float sqrNorm() const			{ return x*x + y*y + z*z + w*w; }
		inline float norm() const				{ return sqrt( sqrNorm() ); }
		inline void	 normalize()				{ ( *this ) /= ( sqrt( sqrNorm() ) ); }

		inline vec3& operator += ( vec3 const& o )	{ x+=o.x; y+=o.y; z+= o.z; w+= o.w; return *this; }
		inline vec3& operator -= ( vec3 const& o )	{ x-=o.x; y-=o.y; z-= o.z; w-= o.w; return *this; }
		inline vec3& operator *= ( vec3 const& o )	{ x*=o.x; y*=o.y; z*= o.z; w*= o.w; return *this; }
		inline vec3& operator /= ( vec3 const& o )	{ x/=o.x; y/=o.y; z/= o.z; w/= o.w; return *this; }
		inline vec3& operator *= ( T c )			{ x*=c;   y*=c;   z*= c;   w*= c;   return *this; }
		inline vec3& operator /= ( T c ) { float cinv = 1.f / c; x*=cinv; y*=cinv; z*=cinv; w*=cinv; return *this; }


	public:
		// Mimic OpenCL cl_float4 alingment
		union
		{
			T CL_ALIGNED( 16 ) s[4];
			struct { T x, y, z, w; };
			struct { T s0, s1, s2, s3; };
		};
	};

	typedef vec3<float> float3;
	typedef vec3<float>	float4;
	typedef vec3<int>	int3;
	typedef	vec3<int>	int4;

	template<typename T>
	inline vec3<T> operator+( vec3<T> const& v1, vec3<T> const&v2 )
	{
		return vec3<T>( v1.x + v2.x, v1.y + v2.y, v1.z + v2.z, v1.w + v2.w );
	}

	template<typename T>
	inline vec3<T> operator-( vec3<T> const& v1, vec3<T> const&v2 )
	{
		return vec3<T>( v1.x - v2.x, v1.y - v2.y, v1.z - v2.z, v1.w - v2.w );
	};

	template<typename T>
	inline vec3<T> operator*( vec3<T> const& v1, vec3<T> const&v2 )
	{
		return vec3<T>( v1.x * v2.x, v1.y * v2.y, v1.z * v2.z, v1.w * v2.w );
	}

	/*template<typename N, typename T>
	inline vec3<T> operator*( vec3<T> const& v1, vec3<N> const&v2 )
	{
		return vec3<T>( static_cast< T >( v1.x * v2.x ), static_cast< T >( v1.y * v2.y ), static_cast< T >( v1.z * v2.z ), static_cast< T >( v1.w * v2.w ) );
	};*/

	template<typename T>
	inline vec3<T> operator/( vec3<T> const& v1, vec3<T> const&v2 )
	{
		return vec3<T>( v1.x / v2.x, v1.y / v2.y, v1.z / v2.z, v1.w / v2.w );
	}

	template<typename T>
	inline vec3<T> operator/( T const& v1, vec3<T> const&v2 )
	{
		return vec3<T>( v1 / v2.x, v1 / v2.y, v1 / v2.z, v1 / v2.w );
	}

	template<typename T>
	inline vec3<T> operator*( vec3<T> const& v1, T c )
	{
		return vec3<T>( v1.x * c, v1.y * c, v1.z * c, v1.w * c );
	}

	template<typename T>
	inline vec3<T> operator*( T c, vec3<T> const& v1 )
	{
		return operator*(v1, c);
	}

	template<typename T>
	inline float dot( vec3<T> const& v1, vec3<T> const& v2)
	{
		return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z + v1.w * v2.w;
	}

	template<typename T>
	inline vec3<T> cross( vec3<T> const& v1, vec3<T> const& v2 )
	{
		return float3( v1.y * v2.z - v2.y * v1.z, v2.x * v1.z - v1.x * v2.z, v1.x * v2.y - v1.y * v2.x );
	}

	template<typename T>
	inline vec3<T> normalize( vec3<T> const& v1)
	{
		vec3<T> v2 = v1;
		v2.normalize();
		return v2;
	}

	template<typename T>
	inline std::ostream& operator<<(std::ostream& stream, float3 const& v1)
	{
		stream << (float)v1.x << ", " << v1.y << ", " << v1.z;
		return stream;
	}

}

#endif // !FLOAT3_H
