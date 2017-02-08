#include "Renderer.h"
#include "TracerTypes.h"
#include "Camera.h"

#include <fstream>
#include <iostream>
#include <string>

#include "tiny_obj_loader.h"

enum Refl_t { DIFF, SPEC, REFR };

struct Sphere
{
	cl_float radius;
	cl_int reflectionType;
	cl_float dummy2;
	cl_float dummy3;
	cl_float4 position;
	cl_float4 color;
	cl_float4 emission;
};

namespace PetTracer
{
	class CRenderer : public Renderer
	{
	public:
		CRenderer(std::string title, unsigned int width, unsigned int height)
			: Renderer(title, width, height),
			  mPerpectiveCamera(float3( 0.0f, 0.0f, 2.0f ), float3(0.0f, 0.0f, 0.0f), float3(0.0f, 1.0f, 0.0f))
		{
			pattern = new float3[mScreenHeight*mScreenWidth];
		}

	protected:
		bool Initialize() override
		{
			bool result = CreateProgram();
			if ( !result ) return false;
			CreateVBO();

			glFinish();

			InitScene( mCPUSpheres );

			mRayBuffer = cl::Buffer( mOpenCLContext, CL_MEM_READ_WRITE, sizeof( CLTypes::Ray ) * mScreenHeight*mScreenWidth );
			mHitBuffer = cl::Buffer( mOpenCLContext, CL_MEM_READ_WRITE, sizeof( CLTypes::Intersection ) * mScreenHeight*mScreenWidth );
			mSpheresBuffer = cl::Buffer( mOpenCLContext, CL_MEM_READ_ONLY, mNumSpheres * sizeof( Sphere ) );
			mAccumBuffer = cl::Buffer( mOpenCLContext, CL_MEM_READ_WRITE, mScreenHeight*mScreenWidth * sizeof( cl_float3 ) );
			mCamera = cl::Buffer( mOpenCLContext, CL_MEM_READ_ONLY, sizeof( Camera ) );
			mQueue.finish();
			mQueue.enqueueWriteBuffer( mCamera, CL_TRUE, 0, sizeof( Camera ), &mPerpectiveCamera );
			mQueue.finish();
			mQueue.enqueueWriteBuffer( mSpheresBuffer, CL_TRUE, 0, mNumSpheres * sizeof( Sphere ), mCPUSpheres );
			mQueue.finish();
			mQueue.enqueueWriteBuffer( mAccumBuffer, CL_TRUE, 0, mScreenHeight*mScreenWidth * sizeof( float3 ), pattern );
			mQueue.finish();
			//clEnqueueFillBuffer( mQueue(), mAccumBuffer(), &pattern, sizeof( pattern ), 0, mScreenHeight*mScreenWidth * sizeof( cl_float3 ), 0, NULL, NULL );
			//mQueue.enqueueFillBuffer<unsigned char>( mAccumBuffer, 0, 0, mScreenHeight*mScreenWidth * sizeof( cl_float3 ) );
			if ( mNumTriangles > 0 )
			{
				mTriangleBuffer = cl::Buffer( mOpenCLContext, CL_MEM_READ_ONLY, mNumTriangles * 3 * sizeof( float3 ) );
				mQueue.finish();
				mQueue.enqueueWriteBuffer( mTriangleBuffer, CL_TRUE, 0, mNumTriangles * 3 * sizeof( float3 ), mSceneData );
				mQueue.finish();
				delete[ ] mSceneData;
			}

			// Initialize camera here

			mVertexBufferGL = cl::BufferGL( mOpenCLContext, CL_MEM_WRITE_ONLY, mVertexBufferObject );
			mVBOs.push_back( mVertexBufferGL );
			mQueue.finish();

			InitializeKernel();

			return true;
		}

		void Draw() override
		{
			mQueue.finish();
			
			static int iteration = 1;
			if ( mResetRender ) { iteration = 1; mResetRender = false; }

			// If the camera has been changed
			if ( mPerpectiveCamera.IsDirty() )
			{
				mPerpectiveCamera.Clean();
				mQueue.enqueueWriteBuffer( mCamera, CL_TRUE, 0, sizeof(Camera), &mPerpectiveCamera );
				mQueue.finish();
				ClearAccumBuffer();
			}

			if ( mResetRender ) { iteration = 1; mResetRender = false; }
			int ran = rand();

			mOpenCLKernel.setArg( 5, (unsigned int) ran );
			mOpenCLKernel.setArg( 6,  iteration );
			mOpenCLKernel.setArg( 8, rand() / RAND_MAX );
			mOpenCLKernel.setArg( 9, rand() / RAND_MAX );
			mQueue.finish();

			//mKPerspectiveCamera.setArg( 3, rand() );
			
			if(mTrace)RunKernel();

			glFinish();
			mQueue.finish();
			
			iteration++;

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
			mQueue.finish();
			glDeleteBuffers( 1, &mVertexBufferObject );
			delete[ ] pattern;
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
			// Wait for any work to finish
			mQueue.finish();
			// Fill the accumulation buffer with 0s
			mQueue.enqueueWriteBuffer( mAccumBuffer, CL_TRUE, 0, mScreenHeight*mScreenWidth * sizeof( float3 ), pattern );
			//clEnqueueFillBuffer( mQueue(), mAccumBuffer(), &pattern, sizeof( pattern ), 0, mScreenHeight*mScreenWidth * sizeof( cl_float3 ), 0, NULL, NULL );
			//mQueue.enqueueFillBuffer( mAccumBuffer, 0, 0, mScreenWidth*mScreenHeight * sizeof( cl_float3 ) );
			mQueue.finish();
			mResetRender = true;
		}

	private:
		void InitializeKernel()
		{
			// Create the kernels from the opencl programs
			mOpenCLKernel = cl::Kernel( mOpenCLProgram, "render_kernel" );
			mKPerspectiveCamera = cl::Kernel( mPCamera, "PerspectiveCamera_GeneratePaths" );
			mKIntersectScene = cl::Kernel( mPTracer, "IntersectClosest" );

			// Generate the base seed for the kernels
			std::srand( static_cast<unsigned int>( time( 0 ) ) );
			unsigned int seed = ( unsigned ) std::rand();

			// Setup each kernel with its data
			mOpenCLKernel.setArg( 0, mSpheresBuffer );
			mOpenCLKernel.setArg( 1, mScreenWidth );
			mOpenCLKernel.setArg( 2, mScreenHeight );
			mOpenCLKernel.setArg( 3, mNumSpheres );
			mOpenCLKernel.setArg( 4, mVertexBufferGL );
			mOpenCLKernel.setArg( 5, seed );
			mOpenCLKernel.setArg( 7, mRayBuffer );
			mOpenCLKernel.setArg( 10, mAccumBuffer );
			mOpenCLKernel.setArg( 11, mTriangleBuffer );
			mOpenCLKernel.setArg( 12, mNumTriangles );

			mKPerspectiveCamera.setArg( 0, mCamera );
			mKPerspectiveCamera.setArg( 1, mScreenWidth );
			mKPerspectiveCamera.setArg( 2, mScreenHeight );
			mKPerspectiveCamera.setArg( 3, seed );
			mKPerspectiveCamera.setArg( 4, mRayBuffer );

			mKIntersectScene.setArg( 0, mSpheresBuffer );
			mKIntersectScene.setArg( 1, mNumSpheres );
			mKIntersectScene.setArg( 2, mRayBuffer );
			mKIntersectScene.setArg( 3, ( unsigned int ) mScreenWidth * mScreenHeight );
			mKIntersectScene.setArg( 4, mHitBuffer );
			mKIntersectScene.setArg( 5, mVertexBufferGL );
			mKIntersectScene.setArg( 6, mScreenWidth );
			mKIntersectScene.setArg( 7, mScreenHeight );
			mQueue.finish();
		}

		void RunKernel()
		{
			// FPS counter
			static auto lastTime = std::chrono::high_resolution_clock::now();
			static int frames = 0;

			auto currentTime = std::chrono::high_resolution_clock::now();
			auto dt = std::chrono::duration_cast< std::chrono::milliseconds >( currentTime - lastTime ).count();

			frames++;
			if ( dt > 2000L )
			{
				std::string title = mTitle + " - " + std::to_string(frames / 2) + " sps";
				SDL_SetWindowTitle( mWindow, title.c_str() );
				frames = 0;
				lastTime = currentTime;
			}

			// every pixel in the image has its own thread or "work item",
			// so the total amount of work items equals the number of pixels
			std::size_t global_work_size = mScreenWidth * mScreenHeight;
			std::size_t local_work_size = 64;//mKIntersectScene.getWorkGroupInfo<CL_KERNEL_WORK_GROUP_SIZE>( mOpenCLDevice );

											 // Ensure the global work size is a multiple of local work size
			if ( global_work_size % local_work_size != 0 )
				global_work_size = ( ( global_work_size ) / local_work_size + 1 ) * local_work_size;
			//std::cout << mScreenWidth * mScreenHeight << " - " << global_work_size << std::endl;


			mQueue.enqueueNDRangeKernel( mKPerspectiveCamera, NULL, global_work_size, local_work_size ); // local_work_size
			mQueue.finish();

			//Make sure OpenGL is done using the VBOs
			glFinish();

			//this passes in the vector of VBO buffer objects 
			mQueue.enqueueAcquireGLObjects( &mVBOs );
			mQueue.finish();

			// launch the kernel
			//for(int i = 0; i < 10; i++) mQueue.enqueueNDRangeKernel( mKIntersectScene, NULL, global_work_size, local_work_size ); // local_work_size
			mQueue.enqueueNDRangeKernel( mOpenCLKernel, NULL, global_work_size, local_work_size ); // local_work_size
			mQueue.finish();

																								   //Release the VBOs so OpenGL can play with them
			mQueue.enqueueReleaseGLObjects( &mVBOs );
			mQueue.finish();
		}

		bool CreateProgram()
		{
			bool err = true;
			err = CreateProgram( "../../../src/kernels/CL/opencl_kernel.cl", mOpenCLProgram ) && err;
			err = CreateProgram( "../../../src/kernels/CL/camera.cl", mPCamera ) && err;
			err = CreateProgram( "../../../src/kernels/CL/tracer.cl", mPTracer ) && err;

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

		void InitScene( Sphere* spheres )
		{
			// Set up camera
			mPerpectiveCamera.SetSensorSize( float2( ( float ) mScreenWidth / mScreenHeight, 1.0f ) * 0.0359999985f );
			mPerpectiveCamera.SetFocalLength( 0.05f );

			// Load and preprocess the obj file
			std::vector<tinyobj::shape_t> shapes;
			std::vector<tinyobj::material_t> materials;
			std::string err = "";

			mSceneData = NULL;
			mNumTriangles = 0;

			// Try to open the file
			err = tinyobj::LoadObj( shapes, materials, "../../../data/teste.obj", "../../../data" );
			if ( err == "" )
			{
				// get the first mesh
				tinyobj::shape_t& shape = shapes[0];
				tinyobj::mesh_t& mesh = shape.mesh;
				mNumTriangles = mesh.indices.size() / 3;
				mSceneData = new float3[mesh.indices.size()];

				std::cout << "Mesh: faces" << mesh.indices.size() << " - " << mNumTriangles << std::endl;

				// Pre process the edges
				for ( unsigned int i = 0; i < mNumTriangles; i++ )
				{
					int id  = mesh.indices[i * 3    ]*3;
					int id2 = mesh.indices[i * 3 + 1]*3;
					int id3 = mesh.indices[i * 3 + 2]*3;

					float3 v1, v2, v3;
					v1 = float3( mesh.positions[id ], mesh.positions[id  + 1], mesh.positions[id  + 2] );
					v2 = float3( mesh.positions[id2], mesh.positions[id2 + 1], mesh.positions[id2 + 2] );
					v3 = float3( mesh.positions[id3], mesh.positions[id3 + 1], mesh.positions[id3 + 2] );

					// Store in scene data
					mSceneData[i * 3    ] = v1;
					mSceneData[i * 3 +1 ] = v2 - v1;
					mSceneData[i * 3 +2 ] = v3 - v1;
				}
			}
			else
			{
				std::cout << "Read file error: " << err << std::endl;
			}

			// left wall
			spheres[0].radius	= 200.0f;
			spheres[0].position = { -200.6f, 0.0f, 0.0f, 0.0f };
			spheres[0].color    = { 0.75f, 0.25f, 0.25f };
			spheres[0].emission = { 0.0f, 0.0f, 0.0f };
			spheres[0].reflectionType = DIFF;

			// right wall
			spheres[1].radius	= 200.0f;
			spheres[1].position = { 200.6f, 0.0f, 0.0f };
			spheres[1].color    = { 0.25f, 0.25f, 0.75f };
			spheres[1].emission = { 0.0f, 0.0f, 0.0f };
			spheres[1].reflectionType = DIFF;

			// floor
			spheres[2].radius	= 200.0f;
			spheres[2].position = { 0.0f, -200.4f, 0.0f };
			spheres[2].color	= { 0.75f, 0.75f, 0.75f };
			spheres[2].emission = { 0.0f, 0.0f, 0.0f };
			spheres[2].reflectionType = DIFF;

			// ceiling
			spheres[3].radius	= 200.0f;
			spheres[3].position = { 0.0f, 200.4f, 0.0f };
			spheres[3].color	= { 0.75f, 0.75f, 0.75f };
			spheres[3].emission = { 0.0f, 0.0f, 0.0f };
			spheres[3].reflectionType = DIFF;

			// back wall
			spheres[4].radius   = 200.0f;
			spheres[4].position = { 0.0f, 0.0f, -200.4f };
			spheres[4].color    = { 0.75f, 0.75f, 0.75f };
			spheres[4].emission = { 0.0f, 0.0f, 0.0f };
			spheres[4].reflectionType = DIFF;

			// front wall 
			spheres[5].radius   = 200.0f;
			spheres[5].position = { 0.0f, 0.0f, 202.0f };
			spheres[5].color    = { 0.75f, 0.75f, 0.75f };
			spheres[5].emission = { 0.0f, 0.0f, 0.0f };
			spheres[5].reflectionType = DIFF;

			// left sphere
			spheres[6].radius   = 0.16f;
			spheres[6].position = { -0.25f, -0.24f, -0.1f };
			spheres[6].color    = { 1.0f, 1.0f, 1.0f };
			spheres[6].emission = { 0.0f, 0.0f, 0.0f };
			spheres[6].reflectionType = DIFF;

			// right sphere
			spheres[7].radius   = 0.16f;
			spheres[7].position = { 0.25f, -0.24f, 0.1f };
			spheres[7].color    = { 1.0f, 1.0f, 1.0f };
			spheres[7].emission = { 0.0f, 0.0f, 0.0f };
			spheres[7].reflectionType = SPEC;

			// lightsource
			spheres[8].radius   = 1.0f;
			spheres[8].position = { 0.0f, 2.36f, 0.0f };
			spheres[8].color    = { 0.0f, 0.0f, 0.0f };
			spheres[8].emission = { 12.0f, 12.0f, 12.0f };
			spheres[8].reflectionType = DIFF;
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

		bool CreateProgram( const std::string& path, cl::Program& program )
		{
			std::string source;
			ReadSourceFile( path, source );

			// Create OpenCL program from source
			program = cl::Program( mOpenCLContext, source.c_str() );

			// Build the program for the selected device
			cl_int result = program.build( { mOpenCLDevice }, " -I../../../src/kernels/CL" );
			{
				cl::string buildLog = program.getBuildInfo<CL_PROGRAM_BUILD_LOG>( mOpenCLDevice );
				std::string logFileName = path + ".buildlog.txt";
				FILE* log;
				fopen_s( &log, logFileName.c_str(), "w" );
				fprintf_s( log, "%s\n", buildLog.c_str() );
				fclose( log );
			}
			if ( result ) std::cout << "ERROR during compilation of: " << path << " (" << result << ") " << std::endl;
			if ( result == CL_BUILD_PROGRAM_FAILURE )
			{
				system( "pause" );
				return false;
			}

			return true;
		}

	private:
		cl::Program		mOpenCLProgram;

		// OpenCL programs
		cl::Program		mPCamera;
		cl::Program		mPTracer;

		// OpenCL kernels
		cl::Kernel		mKPerspectiveCamera;
		cl::Kernel		mKIntersectScene;


		cl::Kernel		mOpenCLKernel;
		cl::Buffer		mSpheresBuffer;
		cl::Buffer		mTriangleBuffer;
		cl::Buffer		mRayBuffer;
		cl::Buffer		mHitBuffer;
		cl::Buffer		mCamera;
		cl::Buffer		mAccumBuffer;

		cl::BufferGL	mVertexBufferGL;
		std::vector<cl::Memory> mVBOs;

		GLuint			mVertexBufferObject;

		unsigned int mNumTriangles;
		float3 *mSceneData = NULL;
		float3 *pattern = NULL;

		static const int mNumSpheres = 9;
		Sphere mCPUSpheres[mNumSpheres];
		Camera mPerpectiveCamera;

		bool mResetRender = false;

		// Camera controll
		bool mRotatingCamera = false;
		float2 mLastMousePosition;
	};
}


int main( int argc, char* argv[] )
{
	PetTracer::CRenderer renderer("OpenCL Path Tracer", 800, 600);
	renderer.Start();
	return 0;
}