#pragma once

#ifndef FLOAT2_H
#define FLOAT2_H

#include <cmath>

namespace PetTracer
{
	template<typename T>
	class vec2
	{
	public:
		vec2( T _x = 0.0f, T _y = 0.0f) : x( _x ), y( _y ) { };

		inline float& operator[]( int i )		{ return s[i]; }
		inline float operator[] ( int i ) const { return s[i]; }
		inline vec2 operator-() const			{ return vec2( -x, -y ); }

		inline float sqrNorm() const			{ return x*x + y*y; };
		inline void	 normalize()				{ ( *this ) /= ( sqrt( sqrNorm() ) ); };

		inline vec2& operator += ( vec2 const& o ) { x+=o.x; y+=o.y; return *this; }
		inline vec2& operator -= ( vec2 const& o ) { x-=o.x; y-=o.y; return *this; }
		inline vec2& operator *= ( vec2 const& o ) { x*=o.x; y*=o.y; return *this; }
		inline vec2& operator /= ( vec2 const& o ) { x/=o.x; y/=o.y; return *this; }
		inline vec2& operator *= ( float c ) { x*=c; y*=c;  return *this; }
		inline vec2& operator /= ( float c ) { float cinv = 1.f / c; x*=cinv; y*=cinv; return *this; }


	public:
		// Mimic OpenCL cl_float2 alingment
		union
		{
			alignas( 8 ) T s[2];
			struct { T x, y; };
			struct { T s0, s1; };
			struct { T lo, hi; };
		};
	};

	typedef vec2<float>		float2;
	typedef vec2<int>		int2;

	template<typename T>
	inline vec2<T> operator+( vec2<T> const& v1, vec2<T> const&v2 )
	{
		return vec2<T>( v1.x + v2.x, v1.y + v2.y );
	}

	template<typename T>
	inline vec2<T> operator-( vec2<T> const& v1, vec2<T> const&v2 )
	{
		return vec2<T>( v1.x - v2.x, v1.y - v2.y );
	}

	template<typename T>
	inline vec2<T> operator*( vec2<T> const& v1, vec2<T> const&v2 )
	{
		return vec2<T>( v1.x * v2.x, v1.y * v2.y );
	}

	template<typename T>
	inline vec2<T> operator/( vec2<T> const& v1, vec2<T> const&v2 )
	{
		return vec2<T>( v1.x / v2.x, v1.y / v2.y );
	}

	template<typename T>
	inline vec2<T> operator/( T const& v1, vec2<T> const&v2 )
	{
		return vec2<T>( v1 / v2.x, v1 / v2.y, v1 / v2.z, v1 / v2.w );
	}

	template<typename T>
	inline vec2<T> operator*( vec2<T> const& v1, T c )
	{
		return vec2<T>( v1.x * c, v1.y * c );
	}

	template<typename T>
	inline vec2<T> operator*( T c, vec2<T> const& v1 )
	{
		return operator*( v1, c );
	}

	template<typename T>
	inline T dot( vec2<T> const& v1, vec2<T> const& v2 )
	{
		return v1.x * v2.x + v1.y * v2.y;
	}

	template<typename T>
	inline vec2<T> normalize( vec2<T> const& v1 )
	{
		vec2 v2 = v1;
		v2.normalize();
		return v2;
	}

}

#endif // !FLOAT2_H
