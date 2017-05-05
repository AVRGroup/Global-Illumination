#ifndef PATH_CL
#define PATH_CL

typedef struct _path
{
	float3 throughput;
	int volume;
	int flags;
	int active;
	int extra1;
} Path;

typedef enum _pathFlags
{
	kNone           = 0x0,
	kKilled         = 0x1,
	kScattered      = 0x2,
	kSpecularBounce = 0x4
} PathFlags;

bool Path_IsScattered( __global Path const* path)
{
	return (path->flags & kScattered);
}

bool Path_IsSpecular( __global Path const* path)
{
	return (path->flags & kSpecularBounce);
}

bool Path_IsAlive(__global Path const* path)
{
	return ((path->flags & kKilled) == 0);
}

void Path_ClearScatterFlag(__global Path* path)
{
	path->flags &= ~kScattered;
}

void Path_SetScatterFlag(__global Path* path)
{
	path->flags |= kScattered;
}

void Path_ClearSpecularFlag(__global Path* path)
{
	path->flags &= ~kSpecularBounce;
}

void Path_SetSpecularFlag(__global Path* path)
{
	path->flags |= kSpecularBounce;
}

void Path_Restart(__global Path* path)
{
	path->flags = 0;
}

int Path_GetVolumeIndex( __global Path const* path)
{
	return path->volume;
}

float3 Path_GetThroughput( __global Path const* path)
{
	float3 t = path->throughput;
	return t;
}

void Path_MulThroughput(__global Path* path, float3 mul)
{
	path->throughput *= mul;
}

void Path_Kill(__global Path* path)
{
	path->flags |= kKilled;
}

void Path_AddContribution( const __global Path* path, __global float3* output, int index, float3 val)
{
	output[index] += Path_GetThroughput(path) * val;
}

#endif