#include "Camera.h"

namespace PetTracer
{
	Camera::Camera( float3 const & eye, float3 const & at, float3 const & up )
		: mPosition( eye ),
		  mAperture( 0.0f ),
		  mFocalDistance( 0.0f),
		  mFocalLength(0.0f),
		  mZCap( 0.0f, 0.0f )
	{
		mForward = normalize(at - eye);
		mRight	 = cross( mForward, normalize( up ) );
		mUp		 = cross( mRight, mForward );
	}

	void Camera::SetFocusDistance( float distance )
	{
		mFocalDistance = distance;
		SetDirty();
	}

	float Camera::GetFocusDistance() const
	{
		return mFocalDistance;
	}

	void Camera::SetFocalLength( float length )
	{
		mFocalLength = length;
		SetDirty();
	}

	float Camera::GetFocalLength() const
	{
		return mFocalLength;
	}

	void Camera::SetAperture( float aperture )
	{
		mAperture = aperture;
		SetDirty();
	}

	float Camera::GetAperture() const
	{
		return mAperture;
	}

	void Camera::SetSensorSize( float2 const & size )
	{
		mDim = size;
		SetDirty();
	}

	cl_float2 Camera::GetSensorSize() const
	{
		return { mDim.x, mDim.y };
	}

	void Camera::SetDepthRange( float2 const & range )
	{
		mZCap = range;
		SetDirty();
	}
	cl_float2 Camera::GetDepthRange() const
	{
		return { mZCap.x, mZCap.y };
	}

	void Camera::Rotate( float angle )
	{
		Rotate( float3( 0.0f, 1.0f, 0.0f ), angle );
		SetDirty();
	}

	void Camera::Tilt( float angle )
	{
		Rotate( mRight, angle );
		SetDirty();
	}

	void Camera::MoveForward( float distance )
	{
		mPosition = mPosition + mForward * distance;
		SetDirty();
	}

	void Camera::MoveRight( float distance )
	{
		mPosition = mPosition + mRight * distance;
		SetDirty();
	}

	void Camera::MoveUp( float distance )
	{
		mPosition = mPosition + mUp * distance;
		SetDirty();
	}


	void Camera::Rotate( float3 & axis, float angle )
	{
		matrix cMatrix = matrix
		(
			mUp.x,		mUp.y,		mUp.z,		0.0f,
			mRight.x,	mRight.y,	mRight.z,	0.0f,
			mForward.x,	mForward.y,	mForward.z,	0.0f,
			0.0f,		0.0f,		0.0f,		1.0f
		);

		// Camera orientation quaternion
		quaternion q = normalize( quaternion( cMatrix ) );

		// Rotate the camera
		q = q * rotation_quaternion( axis, -angle );

		// Return to matrix form
		q.to_matrix( cMatrix );

		// Return to vectors
		mUp		= normalize( float3( cMatrix.m00, cMatrix.m01, cMatrix.m02 ) );
		mRight	= normalize( float3( cMatrix.m10, cMatrix.m11, cMatrix.m12 ) );
		mForward= normalize( float3( cMatrix.m20, cMatrix.m21, cMatrix.m22 ) );
	}

	void Camera::SetDirty()
	{
		mPosition.w = -1.0f;
	}
}
