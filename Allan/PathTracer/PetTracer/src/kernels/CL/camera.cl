#ifndef CAMERA_CL
#define CAMERA_CL

#include <common.cl>
#include <ray.cl>
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
	__global uint* random,
	int frame,
	// Ouput
	__global Ray* rays,
	__global Path* paths
)
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
		__global Path* mPath = paths + gID;

		// Prepare RNG
		Sampler sampler;
#if SAMPLER == RANDOM
		uint scramble = gID * rngSeed;
		Sampler_Init( &sampler, scramble );
#elif SAMPLER == CMJ
		uint rnd = random[gID];
		uint scramble = rnd * 0x1fe3434f * ( ( frame + 133 * rnd ) / ( CMJ_DIM * CMJ_DIM ) );
		Sampler_Init( &sampler, frame % ( CMJ_DIM * CMJ_DIM ), SAMPLE_DIM_CAMERA_OFFSET, scramble );
#endif

		// Need to create a proper sampler
		float2 sample0 = Sampler_Sample2D( &sampler );

		// Calculate [0...1] image plane sample
		float2 imgSample;
		imgSample.x = ( float ) (globalID.x + 0.0001f) / imgWidth + sample0.x / imgWidth; /* Sum random offset for antialias */
		imgSample.y = ( float ) (globalID.y + 0.0001f) / imgHeight + sample0.y / imgHeight;

		// Transform into [-0.5...+0.5] space
		float2 hSample = imgSample - (float2)(0.5f, 0.5f);
		// Transform int [-dim/2.0...+dim/2.0]
		float2 cSample = hSample * camera->dim;

		// Calculate direction to image plane
		mRay->d.xyz = normalize(camera->focalLength * camera->forward + cSample.x * camera->right + cSample.y * camera->up);

		// Ray origin = camera position + nearZ * ray direction
		mRay->o.xyz = camera->p + camera->zcap.x * mRay->d.xyz;
		// Max T value = farZ - nearZ
		mRay->o.w = camera->zcap.y - camera->zcap.x;
		// Generate random time form [0...1]
		mRay->d.w = sample0.x;
		mRay->extra.x = 0xFFFFFFFF;
		mRay->extra.y = 0xFFFFFFFF;
		mRay->padding = 1.0f;

		// Initialize path data
		mPath->throughput = (float3)(1.0f, 1.0f, 1.0f);
		mPath->volume = -1;
		mPath->flags = 0;
		mPath->active = 0xFF;

		//rngs[gID] = rng;
	}
}

#endif