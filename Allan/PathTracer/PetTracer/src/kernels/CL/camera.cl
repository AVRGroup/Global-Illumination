#ifndef CAMERA_CL
#define CAMERA_CL

#include <path.cl>
#include <random.cl>


typedef struct _camera
{
	// Camera coordinate frame
	float3 forward;
	float3 right;
	float3 up;
	// Position
	float3 p;

	// Image plane width and height current units
	float2 dim;

	// Near and far Z
	float2 zcap;
	// Focal lenght
	float focalLength;
	// Camera aspect ratio
	float aspectRatio;
	float focalDistance;
	float apertureRadius;
} Camera;

__kernel
void PerspectiveCamera_GeneratePaths(
							//Future camera description
							__global Camera const* camera,
							// Image resolution
							int imgWidth,
							int imgHeight,
							// RNG seed
							int rngSeed,
							// Sampler data
							// Ouput
							__global Ray* rays)
{
	int gID = get_global_id( 0 );
	int2 globalID;
	globalID.x = gID % imgWidth;
	globalID.y = gID / imgWidth;

	// check for border
	if ( globalID.x < imgWidth && globalID.y < imgHeight )
	{
		// get pointer to the ray
		__global Ray* mRay = rays + ( globalID.y*imgWidth ) + globalID.x;

		// Prepare RNG
		//Rng rng;
		//InitRng(rngSeed + globalID.x * 157 + 10433 * globalID.y, &rng);

		// Need to create a proper sampler
		//float2 sample0 = ( float2 )( RandFloat( &rng ), RandFloat( &rng ) );

		// Calculate [0...1] image plane sample
		float2 imgSample;
		imgSample.x = (float)globalID.x / imgWidth; /* Sum random offset for antialias */
		imgSample.y = (float)globalID.y / imgHeight;

		// Transform into [-0.5...+0.5] space
		float2 hSample = imgSample - (float2)(0.5f, 0.5f);
		// Transform int [-dim/2.0...+dim/2.0]
		float2 cSample = hSample * camera->dim;

		// Calculate direction to image plane
		mRay->d.xyz = normalize(camera->focalLength * camera->forward + cSample.x * camera->right + cSample.y * camera->up);

		// Ray origin = camera position + nearZ * ray direction
		mRay->o.xyz = camera->p + camera->zcap.x * camera->forward;
		// Max T value = farZ - nearZ
		mRay->o.w = camera->zcap.y - camera->zcap.x;
		// Generate random time form [0...1]
		// Not now
	}
}

#endif