#pragma once

#include <CLW.h>

#include "math/MathUtils.h"
;
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
		Buffer( CLWContext const& context, BufferUsage = ReadWrite );
		Buffer( CLWContext const& context, uint64 size, BufferUsage = ReadWrite );
		~Buffer();

		inline uint64 GetSize() const { return mDataSize; };
		inline uint64 GetSizeInBytes() const { return mDataSize * sizeof( T ); };

		// Allocate local memory
		void Alloc( uint64 size );

		void AllocLocalData();
		void DeleteLocalData();

		void AllocDeviceData();
		void DeleteDeviceData();

		void UploadToGPU( bool block, uint64 offsef = 0, bool deleteLocalData = false );
		void DownloadToCPU( bool block, uint64 offset = 0 );

		T const* GetPointer() const;
		T* GetPointer();

		inline CLWBuffer<T>& CLBuffer() { return mDeviceData; };

		template<typename N>
		N const* GetPointerType() const;

		template<typename N>
		N* GetPointerType();

	private:
		uint64				mDataSize;
		// Data that resides on the CPU side
		T*					mLocalData;
		// Data that resides ont the GPU side
		CLWBuffer<T>		mDeviceData;
		// Reference to OpenCL context
		CLWContext const&	mContext;
		// Buffer usage
		BufferUsage			mUsage;
	};

	template<typename T>
	inline Buffer<T>::Buffer( CLWContext const& context, BufferUsage usage )
		: mDataSize( 0 ),
		mLocalData( NULL ),
		mDeviceData(),
		mContext( context ),
		mUsage( usage )
	{
	}

	template<typename T>
	inline Buffer<T>::Buffer( CLWContext const& context, uint64 size, BufferUsage usage )
		: mDataSize( size ),
		mLocalData( NULL ),
		mDeviceData( ),
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
		mDeviceData = CLWBuffer<T>::Create( mContext, mUsage, mDataSize );
	}

	template<typename T>
	void Buffer<T>::DeleteDeviceData()
	{
		mDeviceData = CLWBuffer<T>();
	}

	template<typename T>
	void Buffer<T>::UploadToGPU( bool block, uint64 offset, bool deleteLocalData )
	{
		//queue.enqueueWriteBuffer( *mDeviceData, block, offset, mDataSize * sizeof( T ), mLocalData );
		T* mappedPointer = nullptr;
		mContext.MapBuffer( 0, mDeviceData, CL_MAP_WRITE, offset, mDeviceData.GetElementCount() - offset, &mappedPointer ).Wait();

		memcpy( mappedPointer, mLocalData, GetSizeInBytes() );

		mContext.UnmapBuffer( 0, mDeviceData, mappedPointer ).Wait();

		if ( deleteLocalData )
			DeleteLocalData();
	}

	template<typename T>
	void Buffer<T>::DownloadToCPU( bool block, uint64 offset )
	{
		// Make sure that local data is allocated
		if ( !mLocalData )
			AllocLocalData();

		T* mappedPointer = nullptr;
		mContext.MapBuffer( 0, mDeviceData, CL_MAP_READ, offset, mDeviceData.GetElementCount() - offset, &mappedPointer ).Wait();

		memcpy( mLocalData, mappedPointer, GetSizeInBytes() );

		mContext.UnmapBuffer( 0, mDeviceData, mappedPointer ).Wait();
	}

	template<typename T>
	T const* Buffer<T>::GetPointer() const
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