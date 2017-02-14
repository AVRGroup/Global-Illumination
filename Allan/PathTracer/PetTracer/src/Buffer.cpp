#include "Buffer.h"

namespace PetTracer
{
	template<typename T>
	inline Buffer<T>::Buffer( cl::Context const& context )
		: mDataSize( 0 ),
		  mLocalData( NULL ),
		  mDeviceData( NULL ),
		  mContext( context )
	{
	}

	template<typename T>
	inline Buffer<T>::Buffer( cl::Context const& context, uint64 size )
		: mDataSize(size),
		  mLocalData(NULL),
		  mDeviceData(NULL),
		  mContext( context )
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
		mDeviceData = new cl::Buffer( mContext, NULL, sizeof( T )*mDataSize );
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
		queue.enqueueWriteBuffer( *mDeviceData, block, offset, mDataSize * sizof( T ), mLocalData );

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
	T const * Buffer<T>::GetPointer()
	{
		return mLocalData;
	}

	
	float4 const * Buffer<float4>::GetPointer() { return mLocalData; }
	float2 const * Buffer<float2>::GetPointer() { return mLocalData; }
	float  const * Buffer<float >::GetPointer() { return mLocalData; }
	uint8  const * Buffer<uint8 >::GetPointer() { return mLocalData; }
	uint16 const * Buffer<uint16>::GetPointer() { return mLocalData; }
	uint32 const * Buffer<uint32>::GetPointer() { return mLocalData; }
	uint64 const * Buffer<uint64>::GetPointer() { return mLocalData; }
	int8   const * Buffer<int8  >::GetPointer() { return mLocalData; }
	int16  const * Buffer<int16 >::GetPointer() { return mLocalData; }
	int32  const * Buffer<int32 >::GetPointer() { return mLocalData; }
	int64  const * Buffer<int64 >::GetPointer() { return mLocalData; }
	int4   const * Buffer<int4  >::GetPointer()	{ return mLocalData; }
}