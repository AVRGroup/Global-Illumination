#pragma once

#include "math/MathUtils.h"
#include "../Buffer.h"

namespace PetTracer
{
	class Scene
	{
	public:
		Scene();
		Scene( const char* filePath );
		~Scene();

		inline int4   const * TrianglesIndexesPtr() { return mTrianglesIndexes.GetPointer(); }
		inline float4 const * VerticesPositionPtr() { return mVerticesPosition.GetPointer(); }

		inline uint32 TriangleCount() const { return mTriangleCount; }
		inline uint32 VertexCount() const { return mVertexCount; }

	private:
		// Stats
		uint32			mTriangleCount;
		uint32			mVertexCount;
		// Arrays
		Buffer<int4>	mTrianglesIndexes;
		Buffer<float4>	mVerticesPosition;
		Buffer<float4>	mVerticesNormal;
		Buffer<float4>	mVerticesTexCoord;
	};
}