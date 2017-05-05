#ifndef RANDOM_CL
#define RANDOM_CL

//**** NEW SAMPLER ***/
#define RANDOM 1
#define CMJ 2

#define SAMPLER CMJ
#define CMJ_DIM 16

#define SAMPLE_DIMS_PER_BOUNCE 300
#define SAMPLE_DIM_CAMERA_OFFSET 0
#define SAMPLE_DIM_SURFACE_OFFSET 4
#define SAMPLE_DIM_VOLUME_APPLY_OFFSET 100
#define SAMPLE_DIM_VOLUME_EVALUATE_OFFSET 200

typedef struct _sampler
{
	uint index;
	uint dimension;
	uint scramble;
	uint padding;
} Sampler;

/** Uniform sampler **/
/// Hash function
uint WangHash( uint seed )
{
	seed = ( seed ^ 61 ) ^ ( seed >> 16 );
	seed *= 9;
	seed = seed ^ ( seed >> 4 );
	seed *= 0x27d4eb2d;
	seed = seed ^ ( seed >> 15 );
	return seed;
}

uint UniformSampler_SampleUint(Sampler* sampler)
{
	sampler->index = WangHash( 1664525U * sampler->index + 1013904223U );
	return sampler->index;
}

float UniformSampler_Sample1D(Sampler* sampler)
{
	return ( ( float ) UniformSampler_SampleUint( sampler ) / 0xffffffffU );
}

/** Correllated multi-jittered **/
uint permute(uint i, uint l, uint p)
{
	unsigned w = l - 1;
	w |= w >> 1;
	w |= w >> 2;
	w |= w >> 4;
	w |= w >> 8;
	w |= w >> 16;

	do
	{
		i ^= p;
		i *= 0xe170893d;
		i ^= p >> 16;
		i ^= ( i & w ) >> 4;
		i ^= p >> 8;
		i *= 0x0929eb3f;
		i ^= p >> 23;
		i ^= ( i & w ) >> 1;
		i *= 1 | p >> 27;
		i *= 0x6935fa69;
		i ^= ( i & w ) >> 11;
		i *= 0x74dcb303;
		i ^= ( i & w ) >> 2;
		i *= 0x9e501cc3;
		i ^= ( i & w ) >> 2;
		i *= 0xc860a3df;
		i &= w;
		i ^= i >> 5;
	}
	while ( i >= l );
	return ( i + p ) % l;
}

float randfloat(uint i, uint p)
{
	i ^= p;
	i ^= i >> 17;
	i ^= i >> 10;
	i *= 0xb36534e5;
	i ^= i >> 12;
	i ^= i >> 21;
	i *= 0x93fc4795;
	i ^= 0xdf6e307f;
	i ^= i >> 17;
	i *= 1 | p >> 18;
	return i * ( 1.0f / 4294967808.0f );
}

float2 cmj(int s, int n, int p)
{
	int sx = permute( s % n, n, p * 0xa511e9b3 );
	int sy = permute( s / n, n, p * 0x63d83595 );
	float jx = randfloat( s, p * 0xa399d265 );
	float jy = randfloat( s, p * 0x711ad6a5 );

	return makeFloat2( ( s % n + ( sy + jx ) / n ) / n,
		( s / n + ( sx + jy ) / n ) / n );
}

float2 CmjSampler_Sample2D( Sampler* sampler )
{
	int idx = permute( sampler->index, CMJ_DIM * CMJ_DIM, 0xa399d265 * sampler->dimension * sampler->scramble );
	return cmj( idx, CMJ_DIM, sampler->dimension * sampler->scramble );
}

#if (SAMPLER == RANDOM)
void Sampler_Init(Sampler* sampler, uint seed)
{
	sampler->index = seed;
}
#elif SAMPLER == CMJ
void Sampler_Init(Sampler* sampler, uint index, uint dimension, uint scramble)
{
	sampler->index = index;
	sampler->scramble = scramble;
	sampler->dimension = dimension;
}
#endif

float Sampler_Sample1D(Sampler* sampler)
{
#if SAMPLER == RANDOM
	return UniformSampler_Sample1D( sampler );
#elif SAMPLER == CMJ
	float2 sample;
	sample = CmjSampler_Sample2D( sampler );
	( sampler->dimension )++;
	return sample.x;
#endif
}

float2 Sampler_Sample2D(Sampler* sampler)
{
#if SAMPLER == RANDOM
	float2 sample;
	sample.x = UniformSampler_Sample1D( sampler );
	sample.y = UniformSampler_Sample1D( sampler );
	return sample;
#elif SAMPLER == CMJ
	float2 sample;
	sample = CmjSampler_Sample2D( sampler );
	( sampler->dimension )++;
	return sample;
#endif
}












static float getRandom( unsigned int *seed0, unsigned int *seed1 )
{
	/* hash the seeds using bitwise AND operations and bitshifts */
	*seed0 = 36969 * ( ( *seed0 ) & 65535 ) + ( ( *seed0 ) >> 16 );
	*seed1 = 18000 * ( ( *seed1 ) & 65535 ) + ( ( *seed1 ) >> 16 );
	unsigned int ires = ( ( *seed0 ) << 16 ) + ( *seed1 );
	union
	{
		float f;
		unsigned int ui;
	} res;

	res.ui = ( ires & 0x007fffff ) | 0x40000000;
	return ( res.f - 2.0f ) / 2.0f;
}

typedef struct _rng
{
	uint val;
} Rng;

/// Return random unsigned
uint RandUint( Rng* rng )
{
	rng->val = WangHash( 1664525U * rng->val + 1013904223U );
	return rng->val;
}

/// Return random float
float RandFloat( Rng* rng )
{
	return ( ( float ) RandUint( rng ) ) / 0xffffffffU;
}

/// Initialize RNG
void InitRng( uint seed, Rng* rng )
{
	rng->val = WangHash( seed );
	for ( int i=0; i< 100; ++i )
		RandUint( rng );
}

#endif