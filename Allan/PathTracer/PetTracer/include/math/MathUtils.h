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
	inline float min( float x, float y )
	{
		return ( x < y ) ? x : y;
	}

	inline float max( float x, float y )
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

	inline float4 max( float4 const& f1, float4 const& f2 )
	{
		return float4( max( f1.x, f2.x ), max( f1.y, f2.y ), max( f1.z, f2.z ), max( f1.w, f2.w ));
	};

	inline float4 max( float4 const& f1, float4 const& f2, float4 const& f3 )
	{
		return float4( max(max( f1.x, f2.x ), f3.x), max( max( f1.y, f2.y ), f3.y ), max( max( f1.z, f2.z ), f3.z ), max( max( f1.w, f2.w ), f3.w ) );
	};

	inline float4 min( float4 const& f1, float4 const& f2 )
	{
		return float4( min( f1.x, f2.x ), min( f1.y, f2.y ), min( f1.z, f2.z ), min( f1.w, f2.w ) );
	};

	inline float4 min( float4 const& f1, float4 const& f2, float4 const& f3 )
	{
		return float4( min( min( f1.x, f2.x ), f3.x ), min( min( f1.y, f2.y ), f3.y ), min( min( f1.z, f2.z ), f3.z ), min( min( f1.w, f2.w ), f3.w ) );
	};
}

#endif // !Math_UTILS_H