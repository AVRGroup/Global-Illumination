#include "RenderApp.h"
#include "TracerTypes.h"
#include "Camera.h"
#include "Scene/Scene.h"
#include "Timer.h"

#include "BVH/BVH.h"
#include "BVH/PlainBVHTranslator.h"

#include <fstream>
#include <iostream>
#include <string>
#include <string.h>

#include <ctime>
#include <chrono>
#include <numeric>

#include "tiny_obj_loader.h"

#include <CL/cl.hpp>

namespace PetTracer
{
	class PathTracer : public RenderApp
	{
	public:
		PathTracer(std::string title, unsigned int width, unsigned int height)
			: RenderApp(title, width, height),
			  mPerpectiveCamera(float3( 0.0f, 0.0f, 2.0f ), float3(0.0f, 0.0f, 0.0f), float3(0.0f, 1.0f, 0.0f)), mScene(NULL)
		{
		}

	protected:
		bool Initialize() override
		{
			bool result = CreateProgram();
			if ( !result ) return false;
			CreateVBO();

			glFinish();

			InitScene();
			
			size_t numPixels	= mScreenHeight * mScreenWidth;
			mPP					= CLWParallelPrimitives( mOpenCLContext );
			mRayBuffer[0]		= CLWBuffer<CLTypes::Ray>::Create( mOpenCLContext, CL_MEM_READ_WRITE, numPixels );
			mRayBuffer[1]		= CLWBuffer<CLTypes::Ray>::Create( mOpenCLContext, CL_MEM_READ_WRITE, numPixels );
			mPathBuffer 		= CLWBuffer<CLTypes::Path>::Create( mOpenCLContext, CL_MEM_READ_WRITE, numPixels );
			mIntersections		= CLWBuffer<CLTypes::Intersection>::Create( mOpenCLContext, CL_MEM_READ_WRITE, numPixels );
			mAccumBuffer		= CLWBuffer<float3>::Create( mOpenCLContext, CL_MEM_READ_WRITE, numPixels );
			mCamera				= CLWBuffer<Camera>::Create( mOpenCLContext, CL_MEM_READ_ONLY, 1 );
			mHitCount			= CLWBuffer<int32>::Create( mOpenCLContext, CL_MEM_READ_WRITE, 1 );
			std::vector<uint32> initdata( numPixels );
			std::iota( initdata.begin(), initdata.end(), 0 );
			mIota				= CLWBuffer<int32>::Create( mOpenCLContext, CL_MEM_READ_ONLY, numPixels, &initdata[0] );
			mPixelIndices[0]	= CLWBuffer<int32>::Create( mOpenCLContext, CL_MEM_READ_WRITE, numPixels );
			mPixelIndices[1]	= CLWBuffer<int32>::Create( mOpenCLContext, CL_MEM_READ_WRITE, numPixels );
			mCompactedIndices	= CLWBuffer<int32>::Create( mOpenCLContext, CL_MEM_READ_WRITE, numPixels );
			mHits				= CLWBuffer<int32>::Create( mOpenCLContext, CL_MEM_READ_WRITE, numPixels );
			for ( uint32& n : initdata ) n = rand();
			mRNGState			= CLWBuffer<uint32>::Create( mOpenCLContext, CL_MEM_READ_WRITE, numPixels, &initdata[0] );
			/***/

			UploadCamera();

			ClearAccumBuffer();

			/*{
				cl_int err = CL_SUCCESS;
				auto context  = cl::Context( mOpenCLContext );
				cl::BufferGL( context, CL_MEM_WRITE_ONLY, mVertexBufferObject, &err );
				if ( err != CL_SUCCESS ) std::cout << "Errooo";
			}*/
			mVertexBufferGL = CLWBuffer<float4>::CreateFromGLBuffer( mOpenCLContext, mVertexBufferObject, CL_MEM_WRITE_ONLY, mScreenHeight * mScreenWidth );
			mVBOs.push_back( mVertexBufferGL );

			InitializeKernel();

			return true;
		}

		void Draw() override
		{
			if ( mResetRender ) { mIteration = 1; mResetRender = false; }

			// If the camera has been changed
			if ( mPerpectiveCamera.IsDirty() )
			{
				mPerpectiveCamera.Clean();
				UploadCamera();
				ClearAccumBuffer();
			}

			if ( mResetRender ) { mIteration = 1; mResetRender = false; }
			int ran = rand();

			/*mOpenCLKernel.SetArg( 3, (unsigned int) ran );
			mOpenCLKernel.SetArg( 4,  mIteration );
			mOpenCLKernel.SetArg( 6, rand() / RAND_MAX );
			mOpenCLKernel.SetArg( 7, rand() / RAND_MAX );*/

			mKPerspectiveCamera.SetArg( 3, rand() );
			mKPerspectiveCamera.SetArg( 5, mIteration );
			
			if(mTrace)RunKernel();
			
			mIteration++;

			glClear( GL_COLOR_BUFFER_BIT );
			glBindBuffer( GL_ARRAY_BUFFER, mVertexBufferObject );
			glVertexPointer( 2, GL_FLOAT, sizeof( cl_float3 ), ( GLvoid* ) 0 );
			glColorPointer( 3, GL_UNSIGNED_BYTE, sizeof( cl_float3 ), ( GLvoid* ) ( 2 * sizeof( cl_float ) ) );

			glEnableClientState( GL_VERTEX_ARRAY );
			glEnableClientState( GL_COLOR_ARRAY );
			glDrawArrays( GL_POINTS, 0, mScreenWidth * mScreenHeight );
			glDisableClientState( GL_COLOR_ARRAY );
			glDisableClientState( GL_VERTEX_ARRAY );

			glBindBuffer( GL_ARRAY_BUFFER, 0 );

			SDL_GL_SwapWindow( mWindow );
		}

		void OnShutdow() override
		{
			glFinish();
			mOpenCLContext.Finish( 0 );
			glDeleteBuffers( 1, &mVertexBufferObject );
			delete mScene;
		}

		void KeyDown( SDL_Keycode const& key )
		{
			switch ( key )
			{
			case SDLK_r:
				ClearAccumBuffer();
				break;
			case SDLK_w:
				mPerpectiveCamera.MoveForward( 0.01f );
				break;
			case SDLK_s:
				mPerpectiveCamera.MoveForward( -0.01f );
				break;
			case SDLK_a:
				mPerpectiveCamera.MoveRight( -0.01f );
				break;
			case SDLK_d:
				mPerpectiveCamera.MoveRight( 0.01f );
				break;
			case SDLK_q:
				mPerpectiveCamera.MoveUp( -0.01f );
				break;
			case SDLK_e:
				mPerpectiveCamera.MoveUp( 0.01f );
				break;
			case SDLK_MINUS:
				mPerpectiveCamera.SetFocalLength( mPerpectiveCamera.GetFocalLength() - 0.01f );
				break;
			case SDLK_EQUALS:
				mPerpectiveCamera.SetFocalLength( mPerpectiveCamera.GetFocalLength() + 0.01f );
				break;
			case SDLK_z:
				mPerpectiveCamera.Rotate( 3.141565f * 0.5f );
				break;
			case SDLK_SPACE:
				std::cout << "Camera position: " << mPerpectiveCamera.GetPosition() << std::endl;
			}
		}


		/// Mouse button pressed
		void MouseDown( Uint8 button, Sint32 x, Sint32 y )
		{
			switch ( button )
			{
			case 1:
				mRotatingCamera = true;
				mLastMousePosition = float2( static_cast< float >( x ), static_cast< float >( y ) );
				break;
			}
		}

		/// Mouse button released
		void MouseUp( Uint8 button, Sint32 x, Sint32 y )
		{
			switch ( button )
			{
			case 1:
				mRotatingCamera = false;
				break;
			}
		}

		/// Mouse moved
		void MouseMove( Sint32 x, Sint32 y )
		{
			// if left mouse button is pressed, use the mouse position diference to rotate the camera
			if ( mRotatingCamera )
			{
				float2 mousePosition( static_cast< float >( x ), static_cast< float >( y ) );
				float2 dP = mousePosition - mLastMousePosition;
				// Moving the mouse one height rotates the camera 90 degrees
				dP *= (-1.0f / (mScreenHeight)) * (PI_OVER2);
				mPerpectiveCamera.Rotate( dP.x );
				mPerpectiveCamera.Tilt( dP.y );
				mLastMousePosition = mousePosition;
				//std::cout << "dP: x=" << dP.x << " y=" << dP.y << std::endl;
			}
		}

	protected:
		// Clear the accumulation buffer
		void ClearAccumBuffer()
		{
			// Fill accumulation buffer with zeros
			float3* mappedAccumBuffer = nullptr;
			mOpenCLContext.Flush( 0 );
			mOpenCLContext.MapBuffer( 0, mAccumBuffer, CL_MAP_WRITE, &mappedAccumBuffer ).Wait();
			memset( mappedAccumBuffer, 0, mScreenHeight * mScreenWidth * sizeof( float3 ) );
			mOpenCLContext.UnmapBuffer( 0, mAccumBuffer, mappedAccumBuffer ).Wait();
			
			mResetRender = true;
		}

		void UploadCamera()
		{
			// Compy camera to GPU
			Camera* mappedCamera = nullptr;
			mOpenCLContext.MapBuffer( 0, mCamera, CL_MAP_WRITE, &mappedCamera ).Wait();
			*mappedCamera = mPerpectiveCamera;
			mOpenCLContext.UnmapBuffer( 0, mCamera, mappedCamera ).Wait();
		}

	private:
		void InitializeKernel()
		{
			// Create the kernels from the opencl programs
			//mOpenCLKernel = mOpenCLProgram.GetKernel( "render_kernel" );

			mKPerspectiveCamera = mProgram.GetKernel( "PerspectiveCamera_GeneratePaths" );
			mKIntersectScene    = mProgram.GetKernel( "IntersectClosest" );

			// Generate the base seed for the kernels
			std::srand( static_cast<unsigned int>( time( 0 ) ) );
			unsigned int seed = ( unsigned ) std::rand();

			mKPerspectiveCamera.SetArg( 0, mCamera );
			mKPerspectiveCamera.SetArg( 1, mScreenWidth );
			mKPerspectiveCamera.SetArg( 2, mScreenHeight );
			mKPerspectiveCamera.SetArg( 3, seed );
			mKPerspectiveCamera.SetArg( 4, mRNGState );
			mKPerspectiveCamera.SetArg( 5, mIteration );
			mKPerspectiveCamera.SetArg( 6, mRayBuffer[0] );
			mKPerspectiveCamera.SetArg( 7, mPathBuffer );

			mKIntersectScene.SetArg( 0, mScene->VerticesPositionBuffer() );
			mKIntersectScene.SetArg( 1, mScene->TriangleIndexBuffer() );
			mKIntersectScene.SetArg( 2, mScene->BVHNodeBuffer() );
			mKIntersectScene.SetArg( 3, mRayBuffer[0] );
			mKIntersectScene.SetArg( 4, mHitCount );
			mKIntersectScene.SetArg( 5, mIntersections );
		}

		void RunKernel()
		{
			MeasureFps();

			int32 maxRays = mScreenWidth * mScreenHeight;

			GeneratePrimaryRays();

			// Copy indices
			mOpenCLContext.CopyBuffer( 0, mIota, mPixelIndices[0], 0, 0, mIota.GetElementCount() );
			mOpenCLContext.CopyBuffer( 0, mIota, mPixelIndices[1], 0, 0, mIota.GetElementCount() );
			mOpenCLContext.FillBuffer( 0, mHitCount, maxRays, 1 );


			for ( int32 pass = 0; pass < 5; pass++)
			{
				mOpenCLContext.FillBuffer( 0, mHits, 0, mHits.GetElementCount() );

				// Intersect rays
				TraceRays( pass );

				// Apply scattering
				EvaluateVolume( pass );

				if ( pass > 0 && false ) // have envmap
				{
					// Shade Background
				}

				// Convert intersections to predicates
				FilterPathStream( pass );

				// Compact rays
				mPP.Compact( 0, mHits, mIota, mCompactedIndices, mHitCount );

				// Advance indices to keep pixel indices up to date
				RestorePixelIndices( pass );

				// Shade hits
				//ShadeVolume( pass );

				ShadeSurface( pass );

				// Shade missing rays
				if ( pass == 0 )
					ShadeMiss( pass );


			}
			/*size_t local_work_size = 64;
			size_t global_work_size = ( ( mScreenHeight*mScreenWidth + local_work_size - 1 ) / local_work_size ) * local_work_size;
			mOpenCLContext.Launch1D( 0, global_work_size, local_work_size, mOpenCLKernel );*/
			//mQueue.enqueueNDRangeKernel( mOpenCLKernel, NULL, global_work_size, local_work_size ); // local_work_size


			//Make sure OpenGL is done using the VBOs
			glFinish();
			//this passes in the vector of VBO buffer objects 
			mOpenCLContext.AcquireGLObjects( 0, mVBOs );

			Accumulate();
																								   //Release the VBOs so OpenGL can play with them
			mOpenCLContext.ReleaseGLObjects( 0, mVBOs );
			mOpenCLContext.Finish( 0 );
		}

		bool CreateProgram()
		{
			bool err = true;
			//err = CreateProgram( "../../../src/kernels/CL/opencl_kernel.cl", mOpenCLProgram ) && err;
			//err = CreateProgram( "../../../src/kernels/CL/camera.cl", mPCamera ) && err;
			err = CreateProgram( "../../../src/kernels/CL/tracer.cl", mProgram ) && err;

			return err;
		}

		void CreateVBO()
		{
			glGenBuffers( 1, &mVertexBufferObject );
			glBindBuffer( GL_ARRAY_BUFFER, mVertexBufferObject );

			// Initialize the VBO
			unsigned int size = mScreenWidth * mScreenHeight * sizeof( cl_float3 );
			glBufferData( GL_ARRAY_BUFFER, size, NULL, GL_DYNAMIC_DRAW );
			glBindBuffer( GL_ARRAY_BUFFER, 0 );
		}

		void InitScene()
		{
			// Set up camera
			mPerpectiveCamera.SetSensorSize( float2( ( float ) mScreenWidth / mScreenHeight, 1.0f ) * 0.0359999985f );
			mPerpectiveCamera.SetFocalLength( 0.05f );
			mPerpectiveCamera.SetPosition( float3( 0.0f, 1.0f, 4.2f ) );

			{
				std::cout << std::endl << "Opening Scene" << std::endl;
				Timer<milliseconds> timer;
				mScene = new Scene ( mOpenCLContext, "../../../data/orig.objm" );
				std::cout << "Scene opened: " << timer.ElapsedTime() << "ms elapsed." << std::endl;
				BuildParams params;
				params.MaxLeafSize = 1;
				mScene->BuildBVH( params );
			}
		}

		void ReadSourceFile( const std::string& path, std::string& source )
		{
			source = "";
			std::ifstream file( path, std::ios::binary | std::ios::beg );
			if ( !file )
			{
				std::cout << "\nNo OpenCL file found!" << std::endl << "Exiting..." << std::endl;
				system( "pause" );
			}
			while ( !file.eof() )
			{
				std::string str;
				std::getline( file, str );
				// Avoid commented lines, bug on compiler
				if ( str[0] == '/' && str[1] == '/' ) continue;
				source += str + '\n';
			}
		}

		bool CreateProgram( const std::string& path, CLWProgram& program )
		{
			std::string source;
			ReadSourceFile( path, source );

			// Create OpenCL program from source
			std::vector<char> sourceCode( source.begin(), source.end() );
			program = mOpenCLContext.CreateProgram( sourceCode, " -I../../../src/kernels/CL -cl-fast-relaxed-math -DMAC" );

			return true;
		}

	private:
		void GeneratePrimaryRays()
		{
			size_t local_work_size = 64;
			size_t global_work_size = ( ( mScreenHeight*mScreenWidth + local_work_size - 1 ) / local_work_size ) * local_work_size;

			mOpenCLContext.Launch1D( 0, global_work_size, local_work_size, mKPerspectiveCamera );
		}

		void TraceRays( int32 pass )
		{
			size_t local_work_size = 64;
			size_t global_work_size = ( ( mScreenHeight*mScreenWidth + local_work_size - 1 ) / local_work_size ) * local_work_size;

			mKIntersectScene.SetArg( 3, mRayBuffer[pass & 0x1]);
			mKIntersectScene.SetArg( 4, mHitCount );

			// launch the kernel
			mOpenCLContext.Launch1D( 0, global_work_size, local_work_size, mKIntersectScene ); // local_work_size
		}

		void EvaluateVolume( int32 pass )
		{
			CLWKernel evaluateKernel = mProgram.GetKernel( "EvaluateVolume" );

			int32 arg = 0;
			evaluateKernel.SetArg( arg++, mRayBuffer[pass & 0x1] );
			evaluateKernel.SetArg( arg++, mPixelIndices[( pass + 1 ) & 0x1] );
			evaluateKernel.SetArg( arg++, mHitCount );
			evaluateKernel.SetArg( arg++, 0 );
			evaluateKernel.SetArg( arg++, 0 );
			evaluateKernel.SetArg( arg++, 0 );
			evaluateKernel.SetArg( arg++, rand() );
			evaluateKernel.SetArg( arg++, mRNGState );
			evaluateKernel.SetArg( arg++, 0 );
			evaluateKernel.SetArg( arg++, pass );
			evaluateKernel.SetArg( arg++, mIteration );
			evaluateKernel.SetArg( arg++, mIntersections );
			evaluateKernel.SetArg( arg++, mPathBuffer );
			evaluateKernel.SetArg( arg++, mAccumBuffer );

			{
				size_t local_work_size = 64;
				size_t global_work_size = ( ( mScreenHeight*mScreenWidth + local_work_size - 1 ) / local_work_size ) * local_work_size;
				mOpenCLContext.Launch1D( 0, global_work_size, local_work_size, evaluateKernel );
			}
		}

		void FilterPathStream( int32 pass )
		{
			CLWKernel filterKernel = mProgram.GetKernel( "FilterPathStream" );

			int32 arg = 0;
			filterKernel.SetArg( arg++, mIntersections );
			filterKernel.SetArg( arg++, mHitCount );
			filterKernel.SetArg( arg++, mPixelIndices[(pass+1) & 0x1] );
			filterKernel.SetArg( arg++, mPathBuffer );
			filterKernel.SetArg( arg++, mHits );

			{
				size_t local_work_size = 64;
				size_t global_work_size = ( ( mScreenHeight*mScreenWidth + local_work_size - 1 ) / local_work_size ) * local_work_size;
				mOpenCLContext.Launch1D( 0, global_work_size, local_work_size, filterKernel );
			}
		}

		void RestorePixelIndices( int32 pass )
		{
			CLWKernel restoreKernel = mProgram.GetKernel( "RestorePixelIndices" );

			int32 arg = 0;
			restoreKernel.SetArg( arg++, mCompactedIndices );
			restoreKernel.SetArg( arg++, mHitCount );
			restoreKernel.SetArg( arg++, mPixelIndices[( pass + 1 ) & 0x1] );
			restoreKernel.SetArg( arg++, mPixelIndices[pass & 0x1] );

			{
				size_t local_work_size = 64;
				size_t global_work_size = ( ( mScreenHeight*mScreenWidth + local_work_size - 1 ) / local_work_size ) * local_work_size;
				mOpenCLContext.Launch1D( 0, global_work_size, local_work_size, restoreKernel );
			}
		}

		void ShadeVolume( int32 pass )
		{
			CLWKernel shadeKernel = mProgram.GetKernel( "ShadeVolume" );

			int32 arg = 0;
			shadeKernel.SetArg( arg++, mRayBuffer[pass & 0x1] );
			shadeKernel.SetArg( arg++, mIntersections );
			shadeKernel.SetArg( arg++, mCompactedIndices );
			shadeKernel.SetArg( arg++, mPixelIndices[pass & 0x1] );
			shadeKernel.SetArg( arg++, mHitCount );
			shadeKernel.SetArg( arg++, mScene->VerticesPositionBuffer() );
			shadeKernel.SetArg( arg++, mScene->VerticesNormalBuffer() );
			shadeKernel.SetArg( arg++, mScene->VerticesTexCoordBuffer() );
			shadeKernel.SetArg( arg++, mScene->TriangleIndexBuffer() );
			shadeKernel.SetArg( arg++, 0 );
			shadeKernel.SetArg( arg++, 0 );
			shadeKernel.SetArg( arg++, mScene->MaterialListBuffer() );
			shadeKernel.SetArg( arg++, 0 );
			shadeKernel.SetArg( arg++, 0 );
			shadeKernel.SetArg( arg++, 0 );
			shadeKernel.SetArg( arg++, 0 );
			shadeKernel.SetArg( arg++, 0 ); // Lights
			shadeKernel.SetArg( arg++, 0 ); // Num lights
			shadeKernel.SetArg( arg++, rand() );
			shadeKernel.SetArg( arg++, mRNGState );
			shadeKernel.SetArg( arg++, 0 );
			shadeKernel.SetArg( arg++, pass );
			shadeKernel.SetArg( arg++, mIntersections );
			shadeKernel.SetArg( arg++, 0 ); // Volumes
			shadeKernel.SetArg( arg++, 0 ); // Shadow rays
			shadeKernel.SetArg( arg++, 0 ); // lightsamples
			shadeKernel.SetArg( arg++, mPathBuffer );
			shadeKernel.SetArg( arg++, mRayBuffer[( pass + 1 ) & 0x1] );
			shadeKernel.SetArg( arg++, mAccumBuffer );


			{
				size_t local_work_size = 64;
				size_t global_work_size = ( ( mScreenHeight*mScreenWidth + local_work_size - 1 ) / local_work_size ) * local_work_size;
				mOpenCLContext.Launch1D( 0, global_work_size, local_work_size, shadeKernel );
			}
		}

		void ShadeSurface( int32 pass )
		{
			CLWKernel kernel = mProgram.GetKernel( "ShadeSurface" );
			
			int32 arg = 0;
			kernel.SetArg( arg++, mRayBuffer[pass & 0x1] );
			kernel.SetArg( arg++, mIntersections );
			kernel.SetArg( arg++, mCompactedIndices );
			kernel.SetArg( arg++, mPixelIndices[pass & 0x1] );
			kernel.SetArg( arg++, mHitCount );
			kernel.SetArg( arg++, mScene->VerticesPositionBuffer() );
			kernel.SetArg( arg++, mScene->VerticesNormalBuffer() );
			kernel.SetArg( arg++, mScene->VerticesTexCoordBuffer() );
			kernel.SetArg( arg++, mScene->TriangleIndexBuffer() );
			kernel.SetArg( arg++, 0 ); // Shapes
			kernel.SetArg( arg++, 0 ); // MAterial ids
			kernel.SetArg( arg++, mScene->MaterialListBuffer() );
			kernel.SetArg( arg++, 0 ); // Textures
			kernel.SetArg( arg++, 0 ); // Texture Data
			kernel.SetArg( arg++, 0 ); // Enviroment map
			kernel.SetArg( arg++, 0 ); // envmapmult
			kernel.SetArg( arg++, 0 ); // lights
			kernel.SetArg( arg++, 0 ); // numlights
			kernel.SetArg( arg++, rand() );
			kernel.SetArg( arg++, mRNGState );
			kernel.SetArg( arg++, 0 );
			kernel.SetArg( arg++, pass );
			kernel.SetArg( arg++, mIteration );
			kernel.SetArg( arg++, 0 ); // Volumes
			kernel.SetArg( arg++, 0 ); // Shadow rays
			kernel.SetArg( arg++, 0 ); // lightsamples
			kernel.SetArg( arg++, mPathBuffer );
			kernel.SetArg( arg++, mRayBuffer[( pass + 1 ) & 0x1] );
			kernel.SetArg( arg++, mAccumBuffer );

			// launch the kernel
			{
				size_t local_work_size = 64;
				size_t global_work_size = ( ( mScreenHeight*mScreenWidth + local_work_size - 1 ) / local_work_size ) * local_work_size;
				mOpenCLContext.Launch1D( 0, global_work_size, local_work_size, kernel );
			}
		}

		void ShadeMiss( int32 pass )
		{
			CLWKernel missKernel = mProgram.GetKernel( "ShadeMiss" );

			int32 numRays = mScreenHeight * mScreenWidth;

			int32 arg = 0;
			missKernel.SetArg( arg++, mRayBuffer[pass & 0x1] );
			missKernel.SetArg( arg++, mIntersections );
			missKernel.SetArg( arg++, mPixelIndices[( pass + 1 ) & 0x1] );
			missKernel.SetArg( arg++, numRays );
			missKernel.SetArg( arg++, 0 );
			missKernel.SetArg( arg++, 0 );
			missKernel.SetArg( arg++, 0 );
			missKernel.SetArg( arg++, mPathBuffer );
			missKernel.SetArg( arg++, 0 );
			missKernel.SetArg( arg++, mAccumBuffer );

			// launch the kernel
			/*{
				size_t local_work_size = 64;
				size_t global_work_size = ( ( mScreenHeight*mScreenWidth + local_work_size - 1 ) / local_work_size ) * local_work_size;
				mOpenCLContext.Launch1D( 0, global_work_size, local_work_size, missKernel );
			}*/
		}

		void Accumulate(  )
		{
			size_t local_work_size = 64;
			size_t global_work_size = ( ( mScreenHeight*mScreenWidth + local_work_size - 1 ) / local_work_size ) * local_work_size;

			CLWKernel kernel = mProgram.GetKernel( "Accumulate" );
			int32 arg = 0;
			kernel.SetArg( arg++, mScreenHeight*mScreenWidth );
			kernel.SetArg( arg++, mAccumBuffer );
			kernel.SetArg( arg++, mVertexBufferGL );
			kernel.SetArg( arg++, mScreenWidth );
			kernel.SetArg( arg++, mScreenHeight );
			kernel.SetArg( arg++, mIteration );

			// launch the kernel
			mOpenCLContext.Launch1D( 0, global_work_size, local_work_size, kernel );
		}

		void FillBuffer( CLWBuffer<int32>& buffer, int32 pattern, size_t elements )
		{
			int32* mappedPtr;
			mOpenCLContext.MapBuffer<int32>( 0, buffer, CL_MAP_WRITE, &mappedPtr ).Wait();
			for ( size_t i = 0; i < elements; i++ )
			{
				mappedPtr[i] = pattern;
			}
			mOpenCLContext.UnmapBuffer( 0, buffer, mappedPtr ).Wait();
		}


		void MeasureFps()
		{
			// FPS counter
			static auto lastTime = std::chrono::high_resolution_clock::now();
			static int frames = 0;

			auto currentTime = std::chrono::high_resolution_clock::now();
			auto dt = std::chrono::duration_cast< std::chrono::milliseconds >( currentTime - lastTime ).count();

			frames++;
			if ( dt > 2000L )
			{
				std::string title = mTitle + " - " + std::to_string( frames / 2.0f ) + " sps  -  Sample: " + std::to_string(mIteration);
				SDL_SetWindowTitle( mWindow, title.c_str() );
				frames = 0;
				lastTime = currentTime;
			}
		}


	private:
		//CLWProgram		mOpenCLProgram;

		// OpenCL programs
		//CLWProgram		mPCamera;
		CLWProgram		mProgram;

		// OpenCL kernels
		CLWKernel		mKPerspectiveCamera;
		CLWKernel		mKIntersectScene;

		CLWParallelPrimitives				mPP;


		CLWKernel							mOpenCLKernel;
		CLWBuffer<CLTypes::Ray>				mRayBuffer[2];
		CLWBuffer<CLTypes::Intersection>	mIntersections;
		CLWBuffer<Camera>					mCamera;
		CLWBuffer<float3>					mAccumBuffer;
		
		CLWBuffer<CLTypes::Path>			mPathBuffer;
		CLWBuffer<int32>					mHitCount;
		CLWBuffer<int32>					mHits;
		CLWBuffer<int32>					mIota;
		CLWBuffer<int32>					mPixelIndices[2];
		CLWBuffer<int32>					mCompactedIndices;
		CLWBuffer<uint32>					mRNGState;



		CLWBuffer<float4>					mVertexBufferGL;
		//cl::BufferGL	mVertexBufferGL;
		std::vector<cl_mem>					mVBOs;
		GLuint								mVertexBufferObject;

		int32								mIteration;

		Camera mPerpectiveCamera;

		Scene* mScene;

		bool mResetRender = false;

		// Camera controll
		bool mRotatingCamera = false;
		float2 mLastMousePosition;
	};
}


int main( int argc, char* argv[] )
{
	PetTracer::PathTracer renderer("OpenCL Path Tracer", 800, 600);
	renderer.Start();
	return 0;
}