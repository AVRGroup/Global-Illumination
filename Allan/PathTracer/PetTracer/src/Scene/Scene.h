#pragma once

#include "math/MathUtils.h"
#include "../Buffer.h"

namespace PetTracer
{
	class BVH;
	struct BuildParams;

	class Scene
	{
	public:
		Scene( CLWContext const& context );
		Scene( CLWContext const& context, const char* filePath, bool uploadScene = false );
		~Scene();

		inline int4   const * TrianglesIndexesPtr() { return mTrianglesIndexes.GetPointer(); }
		inline float4 const * VerticesPositionPtr() { return mVerticesPosition.GetPointer(); }

		inline uint32 TriangleCount() const { return mTriangleCount; }
		inline uint32 VertexCount() const { return mVertexCount; }

		bool OpenFile( const char* filePath, bool UploadToGPU = true );
		void UploadScene( bool block = true );

		void BuildBVH( BuildParams const& params );
		void UpdateBVH( BVH const& bvh );
		void LoadBVHFromFile( const char* filename );

		CLWBuffer<int3>& TriangleIndexBuffer()		{ return mTrianglesIndexes.CLBuffer(); }
		CLWBuffer<float3>& VerticesPositionBuffer()	{ return mVerticesPosition.CLBuffer(); }
		CLWBuffer<float3>& VerticesNormalBuffer()		{ return mVerticesNormal.CLBuffer(); }
		CLWBuffer<float3>& VerticesTexCoordBufer()		{ return mVerticesTexCoord.CLBuffer(); }
		CLWBuffer<AABB>& BVHNodeBuffer()				{ return mBVHNodes.CLBuffer(); }

	private:
		std::string		BVHFileName();
		bool			BVHExistis(std::string& path);

	private:
		// Stats
		uint32			mTriangleCount;
		uint32			mVertexCount;
		// Arrays
		Buffer<int4>	mTrianglesIndexes;
		Buffer<float4>	mVerticesPosition;
		Buffer<float4>	mVerticesNormal;
		Buffer<float4>	mVerticesTexCoord;
		Buffer<AABB>	mBVHNodes;
		// OpenCL context
		CLWContext const&	mOpenCLContext;

		std::string		mScenePath;

	};
}