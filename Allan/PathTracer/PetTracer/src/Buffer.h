#pragma once

#define CL_HPP_MINIMUM_OPENCL_VERSION 110
#define CL_HPP_TARGET_OPENCL_VERSION 110
#include <CL/cl.hpp>

#include "math/MathUtils.h"

namespace PetTracer
{
	enum BufferUsage
	{
		ReadWrite = CL_MEM_READ_WRITE,
		WriteOnly = CL_MEM_WRITE_ONLY,
		ReadOnly = CL_MEM_READ_ONLY,
	};

	template<typename T>
	class Buffer
	{
	public:
		Buffer( cl::Context const& context, BufferUsage = ReadWrite );
		Buffer( cl::Context const& context, uint64 size, BufferUsage = ReadWrite );
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

		T const* GetPointer() const;
		T* GetPointer();

		inline cl::Buffer* CLBuffer() { return mDeviceData; };

		template<typename N>
		N const* GetPointerType() const;

		template<typename N>
		N* GetPointerType();

	private:
		uint64			mDataSize;
		// Data that resides on the CPU side
		T*				mLocalData;
		// Data that resides ont the GPU side
		cl::Buffer*		mDeviceData;
		// Reference to OpenCL context
		cl::Context const&	mContext;
		// Buffer usage
		BufferUsage		mUsage;
	};

	template<typename T>
	inline Buffer<T>::Buffer( cl::Context const& context, BufferUsage usage )
		: mDataSize( 0 ),
		mLocalData( NULL ),
		mDeviceData( NULL ),
		mContext( context ),
		mUsage( usage )
	{
	}

	template<typename T>
	inline Buffer<T>::Buffer( cl::Context const& context, uint64 size, BufferUsage usage )
		: mDataSize( size ),
		mLocalData( NULL ),
		mDeviceData( NULL ),
		mContext( context ),
		mUsage( usage )
	{
		Alloc( size );
	}

	template<typename T>
	Buffer<T>::~Buffer()
	{
		DeleteLocalData();
		DeleteDeviceData();
	}

	template<typename T>
	void Buffer<T>::Alloc( uint64 size )
	{
		// If it has diferent sizes, delete all and realloc
		if ( size != mDataSize )
		{
			if ( mDeviceData )
				DeleteDeviceData();
			if ( mLocalData )
				DeleteLocalData();
		}

		mDataSize = size;

		// At this point, if the data is already allocated, they have the same size,
		// so just allocate what is needed
		if ( !mDeviceData )
			AllocDeviceData();
		if ( !mLocalData )
			AllocLocalData();
	}

	template<typename T>
	void Buffer<T>::AllocLocalData()
	{
		mLocalData = new T[mDataSize];
	}

	template<typename T>
	void Buffer<T>::DeleteLocalData()
	{
		delete[ ] mLocalData;
		mLocalData = NULL;
	}

	template<typename T>
	void Buffer<T>::AllocDeviceData()
	{
		mDeviceData = new cl::Buffer( mContext, mUsage, sizeof( T )*mDataSize );
	}

	template<typename T>
	void Buffer<T>::DeleteDeviceData()
	{
		delete mDeviceData;
		mDeviceData = NULL;
	}

	template<typename T>
	void Buffer<T>::UploadToGPU( cl::CommandQueue& queue, bool block, uint64 offset, bool deleteLocalData )
	{
		//queue.enqueueWriteBuffer( *mDeviceData, block, offset, mDataSize * sizeof( T ), mLocalData );
		cl::Event e;
		T* mapPointer = (T*) queue.enqueueMapBuffer( *mDeviceData, false, CL_MAP_WRITE, 0, GetSizeInBytes(), NULL, &e, NULL );
		e.wait();

		memcpy( mapPointer, mLocalData, GetSizeInBytes() );

		queue.enqueueUnmapMemObject( *mDeviceData, mapPointer, NULL, &e );
		e.wait();
		
		if ( deleteLocalData )
			DeleteLocalData();
	}

	template<typename T>
	void Buffer<T>::DownloadToCPU( cl::CommandQueue& queue, bool block, uint64 offset )
	{
		// Make sure that local data is allocated
		if ( !mDeviceData )
			AllocDeviceData();
		queue.enqueueReadBuffer( *mDeviceData, block, offset, mDataSize * sizeof( T ), mLocalData );
	}

	template<typename T>
	T const * Buffer<T>::GetPointer() const
	{
		return mLocalData;
	}

	template<typename T>
	T * Buffer<T>::GetPointer()
	{
		return mLocalData;
	}

	template<typename T>
	template<typename N>
	inline N const * Buffer<T>::GetPointerType() const
	{
		return (T*) mLocalData;
	}

	template<typename T>
	template<typename N>
	inline N * Buffer<T>::GetPointerType()
	{
		return ( T* ) mLocalData;
	}

}