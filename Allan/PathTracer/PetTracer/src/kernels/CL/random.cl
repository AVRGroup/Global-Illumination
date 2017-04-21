#ifndef RANDOM_CL
#define RANDOM_CL

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