#pragma once

#ifndef MATH_UTILS_H
#define MATH_UTILS_H

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
}

#endif // !Math_UTILS_H