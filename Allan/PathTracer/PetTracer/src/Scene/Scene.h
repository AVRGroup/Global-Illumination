#pragma once

#include "math/MathUtils.h"
#include "../Buffer.h"

#include <CL/cl.hpp>

namespace PetTracer
{
	class BVH;
	struct BuildParams;

	class Scene
	{
	public:
		Scene( cl::Context const& context, cl::CommandQueue* queue = NULL );
		Scene( cl::Context const& context, const char* filePath, cl::CommandQueue* queue = NULL, bool uploadScene = false );
		~Scene();

		inline int4   const * TrianglesIndexesPtr() { return mTrianglesIndexes.GetPointer(); }
		inline float4 const * VerticesPositionPtr() { return mVerticesPosition.GetPointer(); }

		inline uint32 TriangleCount() const { return mTriangleCount; }
		inline uint32 VertexCount() const { return mVertexCount; }

		bool OpenFile( const char* filePath, bool UploadToGPU = true );
		void UploadScene(bool block = true, cl::CommandQueue* queue = NULL);

		void BuildBVH( BuildParams const& params );
		void UpdateBVH( BVH const& bvh );
		void LoadBVHFromFile( const char* filename );

		cl::Buffer* TriangleIndexBuffer()		{ return mTrianglesIndexes.CLBuffer(); }
		cl::Buffer* VerticesPositionBuffer()	{ return mVerticesPosition.CLBuffer(); }
		cl::Buffer* VerticesNormalBuffer()		{ return mVerticesNormal.CLBuffer(); }
		cl::Buffer* VerticesTexCoordBufer()		{ return mVerticesTexCoord.CLBuffer(); }
		cl::Buffer* BVHNodeBuffer()				{ return mBVHNodes.CLBuffer(); }

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
		cl::Context const&	mOpenCLContext;
		cl::CommandQueue*	mQueue;

		std::string		mScenePath;

	};
}