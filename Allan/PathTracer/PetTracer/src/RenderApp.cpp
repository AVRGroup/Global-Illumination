#include "RenderApp.h"

#include <vector>
#include <iostream>


#if defined(_WIN32)
	#include <Windows.h>
#elif defined(UNIX)
	#include <GL/glx.h>
#endif

namespace PetTracer
{
	RenderApp::RenderApp( std::string title, unsigned int width, unsigned int height )
		: mTitle(title),
		  mWindow(NULL),
		  mOpenGLContext(NULL),
		  mScreenWidth(width),
		  mScreenHeight(height),
		  mRunning(true),
		  mTrace(true)
	{
	}

	RenderApp::~RenderApp()
	{
		Shutdow();
	}

	void RenderApp::Start()
	{
		if ( Setup() )
		{
			MainLoop();
		}
		else
		{
			//Throw error here
		}
		// In any case, perform a global shutdow
		Shutdow();
	}

	bool RenderApp::Initialize()
	{
		return true;
	}

	void RenderApp::ProcessEvent( SDL_Event & event )
	{
	}

	void RenderApp::PostUpdate()
	{
	}

	void RenderApp::OnShutdow()
	{
	}

	void RenderApp::KeyDown( SDL_Keycode const & key )
	{
	}

	void RenderApp::KeyUp( SDL_Keycode const & key )
	{
	}

	void RenderApp::MouseDown( Uint8 button, Sint32 x, Sint32 y )
	{
	}

	void RenderApp::MouseUp( Uint8 button, Sint32 x, Sint32 y )
	{
	}

	void RenderApp::MouseMove( Sint32 x, Sint32 y )
	{
	}

	bool RenderApp::Setup()
	{
		bool result = true;

		result &= InitializeWindow();
		result &= InitializeOpenGL();
		result &= InitializeOpenCL();
		result &= Initialize();

		return result;
	}

	bool RenderApp::InitializeWindow()
	{
		if ( SDL_Init( SDL_INIT_VIDEO ) < 0 )
		{
			std::string errorMsg = "SDL could not initialize! SDL Error: ";
			errorMsg += SDL_GetError();
			MessageBox( NULL, errorMsg.c_str(), "Error", NULL ); /// TODO Remove windows.h dependencies
			return false;
		}

		mWindow = SDL_CreateWindow( mTitle.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, mScreenWidth, mScreenHeight, SDL_WINDOW_OPENGL );
		if ( mWindow == NULL )
		{
			std::string errorMsg = "Window could not be created! SDL_Error: ";
			errorMsg += SDL_GetError();
			MessageBox( NULL, errorMsg.c_str(), "Error", NULL );
			return false;
		}

		return true;
	}

	bool RenderApp::InitializeOpenGL()
	{
		mOpenGLContext = SDL_GL_CreateContext( mWindow );
		SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY );
		SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 4 );
		SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 3 );
		SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );

		SDL_GL_SetSwapInterval( 1 );

		if ( !gladLoadGL() )
		{
			std::string errorMsg = "Failed to load OpenGL functions";
			MessageBox( NULL, errorMsg.c_str(), "Error", NULL );
			return false;
		}

		return true;
	}

	bool RenderApp::InitializeOpenCL()
	{
		// Find all available OpenCL platforms
		std::vector<CLWPlatform> platforms;
		//std::vector<cl::Device> devices;

		CLWPlatform::CreateAllPlatforms( platforms );

		if ( platforms.size() < 1 )
		{
			std::cout << "OpenCL not supported" << std::endl;
		}

		// Show the names of all available OpenCL platform and its devices
		std::cout << "Available OpenCL platforms: " << std::endl << std::endl;
		for ( unsigned int i = 0; i < platforms.size(); i++ )
		{
			//platforms[i].getDevices( CL_DEVICE_TYPE_ALL, &devices );
			std::cout << "\t" << i + 1 << ": " << platforms[i].GetName() << std::endl;
			for ( unsigned int j = 0; j < platforms[i].GetDeviceCount(); j++ )
			{
				std::cout << "\t\t" << j + 1 << ": " << platforms[i].GetDevice(j).GetName() << std::endl;
				//std::cout << "\t\t\tMax compute units: " << platforms[i].GetDevice( j ).devices[j].getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>() << std::endl;
				std::cout << "\t\t\tMax work group size: " << platforms[i].GetDevice( j ).GetMaxWorkGroupSize() << std::endl;
			}
			std::cout << std::endl;

		}

		// For each platform and its devices, try to create an context with opengl interop
		// Only the device with the OpenGL context will create the context
		unsigned int platformIdx = -1;
		unsigned int deviceIdx = -1;
		for ( unsigned int i = 0; i < platforms.size(); i++ )
		{
			CLWPlatform& Platform = platforms[i];

			for ( unsigned int j = 0; j < Platform.GetDeviceCount(); j++ )
			{
				mOpenCLDevice = Platform.GetDevice(j);
				// Ignore CPU, acuse false GL interop
				if ( mOpenCLDevice.GetType() == CL_DEVICE_TYPE_CPU ) continue;

				if ( mOpenCLDevice.HasGlInterop() )
				{
					// Try to create the context with that device
					cl_context_properties properties[ ] =
					{
					#if defined(_WIN32)
						CL_GL_CONTEXT_KHR, ( cl_context_properties ) wglGetCurrentContext(),
						CL_WGL_HDC_KHR, ( cl_context_properties ) wglGetCurrentDC(),

					#elif defined(UNIX)
						CL_GL_CONTEXT_KHR, ( cl_context_properties ) glXGetCurrentContext(),
						CL_GLX_DISPLAY_KHR, ( cl_context_properties ) glXGetCurrentDisplay(),
					#endif

						CL_CONTEXT_PLATFORM, ( cl_context_properties ) ( cl_platform_id ) Platform,
						0
					};

					// Create an OpenCL context on that device.
					// the context manages all the OpenCL resources
					mOpenCLContext = CLWContext::Create( mOpenCLDevice, properties );
					platformIdx = i;
					deviceIdx = j;
					break;

				}
			}
			if ( platformIdx != -1 ) break;
		}

		if ( platformIdx == -1 ) return false;

		// Print the name of the chosen OpenCL platform
		std::cout << "Using OpenCL platform:  " << platforms[platformIdx].GetName() << std::endl;
		// Print the name of the chosen device
		std::cout << "Using OpenCL device:    " << mOpenCLDevice.GetName() << std::endl << std::endl << std::endl;

		return true;
	}

	void RenderApp::Draw()
	{
	}

	void RenderApp::Update()
	{
	}

	void RenderApp::MainLoop()
	{
		while ( mRunning )
		{
			SDL_Event event;

			// Check for any event
			while ( SDL_PollEvent( &event ) )
			{
				switch ( event.type )
				{
					// Keyboard key pressed
					case SDL_KEYDOWN:
					{
						KeyDown( event.key.keysym.sym );

						// Default key handling
						switch ( event.key.keysym.sym )
						{
						case SDLK_ESCAPE:
							mRunning = false;
							break;
						}
					} break;

					// Keyboard key released
					case SDL_KEYUP:
					{
						KeyUp( event.key.keysym.sym );
					} break;

					// Mouse button down
					case SDL_MOUSEBUTTONDOWN:
					{
						MouseDown( event.button.button, event.button.x, event.button.y );
					} break;

					// Mouse button up
					case SDL_MOUSEBUTTONUP:
					{
						MouseUp( event.button.button, event.button.x, event.button.y );
					} break;

					// Mouse move
					case SDL_MOUSEMOTION:
					{
						MouseMove( event.button.x, event.button.y );
					} break;

					// X button
					case SDL_QUIT:
						mRunning = false;
						break;
				}

				// For custom events
				ProcessEvent( event );
			}

			// For each component call their update function here
			Update();

			Draw();

			// Controll sync here
			PostUpdate();
		}
	}

	void RenderApp::Shutdow()
	{
		// Call components shutdow function here
		glFinish();

		OnShutdow();

		if ( mOpenGLContext )	SDL_GL_DeleteContext( mOpenGLContext );
		if ( mWindow ) SDL_DestroyWindow( mWindow );
		SDL_Quit();
	}

}
