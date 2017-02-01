#pragma once

#ifndef FLOAT2_H
#define FLOAT2_H

#include <CL/cl.h>
#include <cmath>

namespace PetTracer
{
	class float2
	{
	public:
		float2( float _x = 0.0f, float _y = 0.0f) : x( _x ), y( _y ) { };

		inline float& operator[]( int i )		{ return s[i]; }
		inline float operator[] ( int i ) const { return s[i]; }
		inline float2 operator-() const			{ return float2( -x, -y ); }

		inline float sqrNorm() const			{ return x*x + y*y; };
		inline void	 normalize()				{ ( *this ) /= ( sqrt( sqrNorm() ) ); };

		inline float2& operator += ( float2 const& o ) { x+=o.x; y+=o.y; return *this; }
		inline float2& operator -= ( float2 const& o ) { x-=o.x; y-=o.y; return *this; }
		inline float2& operator *= ( float2 const& o ) { x*=o.x; y*=o.y; return *this; }
		inline float2& operator /= ( float2 const& o ) { x/=o.x; y/=o.y; return *this; }
		inline float2& operator *= ( float c ) { x*=c; y*=c;  return *this; }
		inline float2& operator /= ( float c ) { float cinv = 1.f / c; x*=cinv; y*=cinv; return *this; }


	public:
		// Mimic OpenCL cl_float2 alingment
		union
		{
			cl_float CL_ALIGNED( 8 ) s[2];
			struct { cl_float x, y; };
			struct { cl_float s0, s1; };
			struct { cl_float lo, hi; };
		};
	};

	inline float2 operator+( float2 const& v1, float2 const&v2 )
	{
		return float2( v1.x + v2.x, v1.y + v2.y );
	}

	inline float2 operator-( float2 const& v1, float2 const&v2 )
	{
		return float2( v1.x - v2.x, v1.y - v2.y );
	}

	inline float2 operator*( float2 const& v1, float2 const&v2 )
	{
		return float2( v1.x * v2.x, v1.y * v2.y );
	}

	inline float2 operator/( float2 const& v1, float2 const&v2 )
	{
		return float2( v1.x / v2.x, v1.y / v2.y );
	}

	inline float2 operator*( float2 const& v1, float c )
	{
		return float2( v1.x * c, v1.y * c );
	}

	inline float2 operator*( float c, float2 const& v1 )
	{
		return operator*( v1, c );
	}

	inline float dot( float2 const& v1, float2 const& v2 )
	{
		return v1.x * v2.x + v1.y * v2.y;
	}

	inline float2 normalize( float2 const& v1 )
	{
		float2 v2 = v1;
		v2.normalize();
		return v2;
	}

}

#endif // !FLOAT2_H
