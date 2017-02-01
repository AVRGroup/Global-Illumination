#include "Renderer.h"

#include <vector>
#include <iostream>


#if defined(_WIN32)
	#include <Windows.h>
#elif defined(UNIX)
	#include <GL/glx.h>
#endif

namespace PetTracer
{
	Renderer::Renderer( std::string title, unsigned int width, unsigned int height )
		: mTitle(title),
		  mWindow(NULL),
		  mOpenGLContext(NULL),
		  mScreenWidth(width),
		  mScreenHeight(height),
		  mRunning(true),
		  mTrace(true)
	{
	}

	Renderer::~Renderer()
	{
		Shutdow();
	}

	void Renderer::Start()
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

	bool Renderer::Initialize()
	{
		return true;
	}

	void Renderer::ProcessEvent( SDL_Event & event )
	{
	}

	void Renderer::PostUpdate()
	{
	}

	void Renderer::OnShutdow()
	{
	}

	void Renderer::KeyDown( SDL_Keycode const & key )
	{
	}

	void Renderer::KeyUp( SDL_Keycode const & key )
	{
	}

	void Renderer::MouseDown( Uint8 button, Sint32 x, Sint32 y )
	{
	}

	void Renderer::MouseUp( Uint8 button, Sint32 x, Sint32 y )
	{
	}

	void Renderer::MouseMove( Sint32 x, Sint32 y )
	{
	}

	bool Renderer::Setup()
	{
		bool result = true;

		result &= InitializeWindow();
		result &= InitializeOpenGL();
		result &= InitializeOpenCL();
		result &= Initialize();

		return result;
	}

	bool Renderer::InitializeWindow()
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

	bool Renderer::InitializeOpenGL()
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

	bool Renderer::InitializeOpenCL()
	{
		// Find all available OpenCL platforms
		std::vector<cl::Platform> platforms;
		std::vector<cl::Device> devices;


		cl::Platform::get( &platforms );

		// Show the names of all available OpenCL platform and its devices
		std::cout << "Available OpenCL platforms: " << std::endl << std::endl;
		for ( unsigned int i = 0; i < platforms.size(); i++ )
		{
			devices.clear();
			platforms[i].getDevices( CL_DEVICE_TYPE_ALL, &devices );
			std::cout << "\t" << i + 1 << ": " << platforms[i].getInfo<CL_PLATFORM_NAME>() << std::endl;
			for ( unsigned int j = 0; j < devices.size(); j++ )
			{
				std::cout << "\t\t" << j + 1 << ": " << devices[j].getInfo<CL_DEVICE_NAME>() << std::endl;
				std::cout << "\t\t\tMax compute units: "  << devices[j].getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>() << std::endl;
				std::cout << "\t\t\tMax work group size: " << devices[j].getInfo<CL_DEVICE_MAX_WORK_GROUP_SIZE>() << std::endl;
			}
			std::cout << std::endl;

		}

		// For each platform and its devices, try to create an context with opengl interop
		// Only the device with the OpenGL context will create the context
		unsigned int platformIdx = -1;
		unsigned int deviceIdx = -1;
		for ( unsigned int i = 0; i < platforms.size(); i++ )
		{
			mOpenCLPlatform = platforms[i];
			devices.clear();
			mOpenCLPlatform.getDevices( CL_DEVICE_TYPE_ALL, &devices );

			for ( unsigned int j = 0; j < devices.size(); j++ )
			{
				mOpenCLDevice = devices[j];
				if ( mOpenCLDevice.getInfo<CL_DEVICE_TYPE>() == CL_DEVICE_TYPE_CPU ) continue;

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

					CL_CONTEXT_PLATFORM, ( cl_context_properties ) mOpenCLPlatform(),
					0
				};

				// Create an OpenCL context on that device.
				// the context manages all the OpenCL resources
				cl_int err = 0;
				mOpenCLContext = cl::Context( mOpenCLDevice, properties, NULL, NULL, &err);
				
				// If there is no error, exit the loop
				if ( err == CL_SUCCESS )
				{
					platformIdx = i;
					deviceIdx = j;
					break;
				}
			}
			if ( platformIdx != -1 ) break;
		}

		// Print the name of the chosen OpenCL platform
		std::cout << "Using OpenCL platform:  " << mOpenCLPlatform.getInfo<CL_PLATFORM_NAME>() << std::endl;
		// Print the name of the chosen device
		std::cout << "Using OpenCL device:    " << mOpenCLDevice.getInfo<CL_DEVICE_NAME>() << std::endl << std::endl << std::endl;

		mQueue = cl::CommandQueue( mOpenCLContext, mOpenCLDevice );

		return true;
	}

	void Renderer::Draw()
	{
	}

	void Renderer::Update()
	{
	}

	void Renderer::MainLoop()
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

	void Renderer::Shutdow()
	{
		// Call components shutdow function here
		glFinish();

		OnShutdow();

		if ( mOpenGLContext )	SDL_GL_DeleteContext( mOpenGLContext );
		if ( mWindow ) SDL_DestroyWindow( mWindow );
		SDL_Quit();
	}

}
