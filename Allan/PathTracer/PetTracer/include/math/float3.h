#pragma once

#ifndef FLOAT3_H
#define FLOAT3_H

#include <CL/cl.h>
#include <cmath>
#include <iostream>

namespace PetTracer
{
	class float3
	{
	public:
		float3( float _x = 0.0f, float _y = 0.0f, float _z = 0.0f, float _w = 0.0f ) : x( _x ), y( _y ), z( _z ), w( _w ) { };

		inline float& operator[]( int i )		{ return s[i]; }
		inline float operator[]( int i ) const	{ return s[i]; }
		inline float3 operator-() const			{ return float3( -x, -y, -z, -w ); }

		inline float sqrNorm() const			{ return x*x + y*y + z*z + w*w; }
		inline float norm() const				{ return sqrt( sqrNorm() ); }
		inline void	 normalize()				{ ( *this ) /= ( sqrt( sqrNorm() ) ); }

		inline float3& operator += ( float3 const& o )	{ x+=o.x; y+=o.y; z+= o.z; w+= o.w; return *this; }
		inline float3& operator -= ( float3 const& o )	{ x-=o.x; y-=o.y; z-= o.z; w-= o.w; return *this; }
		inline float3& operator *= ( float3 const& o )	{ x*=o.x; y*=o.y; z*= o.z; w*= o.w; return *this; }
		inline float3& operator /= ( float3 const& o )	{ x/=o.x; y/=o.y; z/= o.z; w/= o.w; return *this; }
		inline float3& operator *= ( float c )			{ x*=c;   y*=c;   z*= c;   w*= c;   return *this; }
		inline float3& operator /= ( float c ) { float cinv = 1.f / c; x*=cinv; y*=cinv; z*=cinv; w*=cinv; return *this; }


	public:
		// Mimic OpenCL cl_float4 alingment
		union
		{
			cl_float CL_ALIGNED( 16 ) s[4];
			struct { cl_float x, y, z, w; };
			struct { cl_float s0, s1, s2, s3; };
			struct { cl_float2 lo, hi; };
		};
	};

	typedef float3 float4;

	inline float3 operator+( float3 const& v1, float3 const&v2 )
	{
		return float3( v1.x + v2.x, v1.y + v2.y, v1.z + v2.z, v1.w + v2.w );
	}

	inline float3 operator-( float3 const& v1, float3 const&v2 )
	{
		return float3( v1.x - v2.x, v1.y - v2.y, v1.z - v2.z, v1.w - v2.w );
	}

	inline float3 operator*( float3 const& v1, float3 const&v2 )
	{
		return float3( v1.x * v2.x, v1.y * v2.y, v1.z * v2.z, v1.w * v2.w );
	}

	inline float3 operator/( float3 const& v1, float3 const&v2 )
	{
		return float3( v1.x / v2.x, v1.y / v2.y, v1.z / v2.z, v1.w / v2.w );
	}

	inline float3 operator*( float3 const& v1, float c )
	{
		return float3( v1.x * c, v1.y * c, v1.z * c, v1.w * c );
	}

	inline float3 operator*( float c, float3 const& v1 )
	{
		return operator*(v1, c);
	}

	inline float dot(float3 const& v1, float3 const& v2)
	{
		return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z + v1.w * v2.w;
	}

	inline float3 cross( float3 const& v1, float3 const& v2 )
	{
		return float3( v1.y * v2.z - v2.y * v1.z, v2.x * v1.z - v1.x * v2.z, v1.x * v2.y - v1.y * v2.x );
	}

	inline float3 normalize( float3 const& v1)
	{
		float3 v2 = v1;
		v2.normalize();
		return v2;
	}

	inline std::ostream& operator<<(std::ostream& stream, float3 const& v1)
	{
		stream << (float)v1.x << ", " << v1.y << ", " << v1.z;
		return stream;
	}

}

#endif // !FLOAT3_H
