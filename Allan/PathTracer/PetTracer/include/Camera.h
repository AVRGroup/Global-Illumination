#pragma once

#ifndef CAMERA_H
#define CAMERA_H

#include <CL/opencl.h>

#include "math/MathUtils.h"

namespace PetTracer
{
	class Camera
	{
	public:
		Camera( float3 const& eye, float3 const& at, float3 const& up);

		// Set camera focus distance in meters,
		// this is essentialy a distance from  the lens to the focal plane.
		// Altering this is simila to rotating the focus ring on real lens.
		void SetFocusDistance( float distance );
		float GetFocusDistance() const;

		// Set camera focal legth in meters.
		// This is essentialy a distance between a camera sensor and a lens.
		// Altering this is similar to rotating zoom ring on a lens.
		void SetFocalLength( float length );
		float GetFocalLength() const;

		// Set aperture value in meters.
		// This is a radius of a lens.
		void SetAperture( float aperture );
		float GetAperture() const;

		// Set camera sensor size in meters.
		// This distinguishes APC-S vs full-frame, etc 
		void SetSensorSize( float2 const& size );
		cl_float2 GetSensorSize() const;

		// Set camera depth range.
		// Does not really make sence for physical camera
		void SetDepthRange( float2 const& range );
		cl_float2 GetDepthRange() const;

		
		// Rotate camera around world Z axis
		void Rotate( float angle );
		// Tilt camera
		void Tilt( float angle );
		// Move camera along Z direction
		void MoveForward( float distance );
		// Move camera along X direction
		void MoveRight( float distance);
		// Move camera along Y direction
		void MoveUp( float distance );

		// 
		void ArcballRotateHorizontally( cl_float3 const& c, float angle );
		//
		void ArcballRotateVertically( cl_float3 const& c, float angle );

		inline bool IsDirty() const { return mPosition.w == -1.0f; };
		inline void Clean() { mPosition.w = 0.0f; };



	private:
		void Rotate( float3& axis, float angle );
		void SetDirty();

		// Camera coordinate frame
		float3 mForward;
		float3 mRight;
		float3 mUp;
		// Position
		float3 mPosition;

		// Image plane width and height current units
		float2 mDim;

		// Near and far Z
		float2 mZCap;
		// Focal lenght
		float mFocalLength;
		// Camera aspect ratio
		float mAspectRatio;
		float mFocalDistance;
		float mAperture;

	};
}

#endif