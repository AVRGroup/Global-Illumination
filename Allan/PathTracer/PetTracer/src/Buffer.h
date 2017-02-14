#pragma once

#define CL_HPP_MINIMUM_OPENCL_VERSION 110
#define CL_HPP_TARGET_OPENCL_VERSION 110
#include <CL/cl.hpp>

#include "math/MathUtils.h"

namespace PetTracer
{
	template<typename T>
	class Buffer
	{
	public:
		Buffer( cl::Context const& context );
		Buffer( cl::Context const& context, uint64 size );
		~Buffer();

		inline uint64 GetSize() const { return mDataSize; };
		inline uint64 GetSizeInBytes() const { return mDataSize * sizeof( T ); };

		// Allocate local memory
		void Alloc( uint64 size );

		void AllocLocalData();
		void DeleteLocalData();

		void AllocDeviceData();
		void DeleteDeviceData();

		void UploadToGPU( cl::CommandQueue& queue, bool block, uint64 offsef = 0, bool deleteLocalData = false );
		void DownloadToCPU( cl::CommandQueue& queue, bool block, uint64 offset = 0 );

		T const* GetPointer();

		template<typename N>
		N const* GetPointerType();

	private:
		uint64			mDataSize;
		// Data that resides on the CPU side
		T*				mLocalData;
		// Data that resides ont the GPU side
		cl::Buffer*		mDeviceData;
		// Reference to OpenCL context
		cl::Context&	mContext;
	};

	template<typename T>
	template<typename N>
	inline N const * Buffer<T>::GetPointerType()
	{
		return (T*) mLocalData;
	}
}