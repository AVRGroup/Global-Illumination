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
			mHitCount[0]		= CLWBuffer<int32>::Create( mOpenCLContext, CL_MEM_READ_WRITE, 1 );
			mHitCount[1]		= CLWBuffer<int32>::Create( mOpenCLContext, CL_MEM_READ_WRITE, 1 );
			mRNGState			= CLWBuffer<uint32>::Create( mOpenCLContext, CL_MEM_READ_WRITE, numPixels );
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

			// Setup each kernel with its data
			/*mOpenCLKernel.SetArg( 0,  mScreenWidth );
			mOpenCLKernel.SetArg( 1,  mScreenHeight );
			mOpenCLKernel.SetArg( 2,  mVertexBufferGL );
			mOpenCLKernel.SetArg( 3,  seed );
			mOpenCLKernel.SetArg( 5,  mRayBuffer[0] );
			mOpenCLKernel.SetArg( 8,  mAccumBuffer );
			mOpenCLKernel.SetArg( 9,  mScene->VerticesPositionBuffer() );
			mOpenCLKernel.SetArg( 10, mScene->TriangleIndexBuffer() );
			mOpenCLKernel.SetArg( 11, mScene->BVHNodeBuffer() );*/

			mKPerspectiveCamera.SetArg( 0, mCamera );
			mKPerspectiveCamera.SetArg( 1, mScreenWidth );
			mKPerspectiveCamera.SetArg( 2, mScreenHeight );
			mKPerspectiveCamera.SetArg( 3, seed );
			mKPerspectiveCamera.SetArg( 4, mRNGState );
			mKPerspectiveCamera.SetArg( 5, mRayBuffer[0] );
			mKPerspectiveCamera.SetArg( 6, mPathBuffer );

			mKIntersectScene.SetArg( 0, mScene->VerticesPositionBuffer() );
			mKIntersectScene.SetArg( 1, mScene->TriangleIndexBuffer() );
			mKIntersectScene.SetArg( 2, mScene->BVHNodeBuffer() );
			mKIntersectScene.SetArg( 3, mRayBuffer[0] );
			mKIntersectScene.SetArg( 4, mScreenHeight*mScreenWidth );
			mKIntersectScene.SetArg( 5, mIntersections );
			mKIntersectScene.SetArg( 6, mVertexBufferGL );
			mKIntersectScene.SetArg( 7, mScreenWidth );
			mKIntersectScene.SetArg( 8, mScreenHeight );
			mKIntersectScene.SetArg( 9, mHitCount[0] );
		}

		void RunKernel()
		{
			MeasureFps();

			int32 maxRays = mScreenWidth * mScreenHeight;

			GeneratePrimaryRays();

			// Copy indices
			FillBuffer( mHitCount[0], maxRays, 1 );


			for ( int32 pass = 0; pass < 5; pass++)
			{

				TraceRays( pass );

				FillBuffer( mHitCount[(pass+1) & 0x1], 0, 1 );

				ShadeSurface( pass );


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

			{
				std::cout << std::endl << "Opening Scene" << std::endl;
				Timer<milliseconds> timer;
				mScene = new Scene ( mOpenCLContext, "../../../data/orig2.obj" );
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

		void TraceRays(int pass)
		{
			size_t local_work_size = 64;
			size_t global_work_size = ( ( mScreenHeight*mScreenWidth + local_work_size - 1 ) / local_work_size ) * local_work_size;

			mKIntersectScene.SetArg( 3, mRayBuffer[pass & 0x1]);
			mKIntersectScene.SetArg( 4, mHitCount[pass & 0x1] );

			// launch the kernel
			mOpenCLContext.Launch1D( 0, global_work_size, local_work_size, mKIntersectScene ); // local_work_size
		}

		void ShadeSurface( int pass )
		{
			size_t local_work_size = 64;
			size_t global_work_size = ( ( mScreenHeight*mScreenWidth + local_work_size - 1 ) / local_work_size ) * local_work_size;

			CLWKernel kernel = mProgram.GetKernel( "ShadeSurface" );
			int32 arg = 0;
			kernel.SetArg( arg++, mRayBuffer[pass & 0x1] );
			kernel.SetArg( arg++, mRayBuffer[( pass + 1 ) & 0x1] );
			kernel.SetArg( arg++, mIntersections );
			kernel.SetArg( arg++, mHitCount[pass & 0x1] );
			kernel.SetArg( arg++, mHitCount[( pass + 1 ) & 0x1] );
			kernel.SetArg( arg++, mScene->VerticesPositionBuffer() );
			kernel.SetArg( arg++, mScene->VerticesNormalBuffer() );
			kernel.SetArg( arg++, mScene->VerticesTexCoordBuffer() );
			kernel.SetArg( arg++, mScene->TriangleIndexBuffer() );
			kernel.SetArg( arg++, mScene->MaterialListBuffer() );
			kernel.SetArg( arg++, rand() );
			kernel.SetArg( arg++, mRNGState );
			kernel.SetArg( arg++, pass );
			kernel.SetArg( arg++, mPathBuffer );
			kernel.SetArg( arg++, mAccumBuffer );

			// launch the kernel
			mOpenCLContext.Launch1D( 0, global_work_size, local_work_size, kernel );
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
				std::string title = mTitle + " - " + std::to_string( frames / 2.0f ) + " sps";
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
		CLWBuffer<int>						mHitCount[2];
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
	PetTracer::PathTracer renderer("OpenCL Path Tracer", 500, 500);
	renderer.Start();
	return 0;
}