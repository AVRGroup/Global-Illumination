#if defined(_MSC_VER)
#pragma once
#endif

#ifndef PETTRACER_RENDERER_H
#define PETTRACER_RENDERER_H

#include <CLW.h>
#include <glad/glad.h>
#include <SDL.h>
#include <SDL_opengl.h>

//#include <CL/cl.hpp>

#include <string>


namespace PetTracer
{
	class RenderApp
	{
	public:
		RenderApp(std::string Title, unsigned int width, unsigned int height);
		virtual ~RenderApp();

		void Start();


	protected:
		virtual bool Initialize();
		virtual void ProcessEvent( SDL_Event& event );
		virtual void Draw();
		virtual void PostUpdate();
		virtual void OnShutdow();

		virtual void KeyDown(SDL_Keycode const& key);
		virtual void KeyUp(SDL_Keycode const& key);
		virtual void MouseDown( Uint8 button, Sint32 x, Sint32 y );
		virtual void MouseUp( Uint8 button, Sint32 x, Sint32 y );
		virtual void MouseMove( Sint32 x, Sint32 y );

	private:
		// Internal function called at start, calls the virtual Initialize and the internal initialize
		bool Setup();
		bool InitializeWindow();
		bool InitializeOpenGL();
		bool InitializeOpenCL();

		// Internal Draw function, calls opencl and components draws functions
		//void Draw();

		// Internal update function, call all components update function
		void Update();

		void MainLoop();

		void Shutdow();

	protected:
		std::string			mTitle;

		SDL_Window*			mWindow;
		SDL_GLContext		mOpenGLContext;

		unsigned int		mScreenWidth;
		unsigned int		mScreenHeight;

		
		CLWDevice			mOpenCLDevice;
		CLWContext			mOpenCLContext;
		//cl::Platform		mOpenCLPlatform;
		//cl::Device		mOpenCLDevice;
		//cl::Context		mOpenCLContext;
		//cl::CommandQueue	mQueue;

		bool				mRunning;
		bool				mTrace;

	};
}

#endif // !PETTRACER_RENDERER_H

