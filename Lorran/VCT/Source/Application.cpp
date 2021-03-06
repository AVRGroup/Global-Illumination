#include "Application.h"

// Standard library.
#include <iostream>
#include <iomanip>
#include <time.h>

// External.
#define GLEW_STATIC
//#include <GL/glew.h>
#include "../Include/GL/glew.h"
#include "../Include/GLFW/glfw3.h"
//#include <GLFW/glfw3.h>

// Internal.
#include "Scene/Scene.h"
#include "Scene/ScenePack.h"
#include "Graphic/Graphics.h"
#include "Graphic/Material/MaterialStore.h"
#include "Graphic/Renderer/MeshRenderer.h"
#include "Time/Time.h"

#define __LOG_INTERVAL 0 /* How often we should log frame rate info to the console. = 0 means don't log. */
#if __LOG_INTERVAL > 0
constexpr float __LOG_INTERVAL_TIME_GUARD = 1.0f;
#endif

using __DEFAULT_LEVEL = GlassScene; // The scene that will be loaded on startup.
// (see ScenePack.h for more scenes) CornellScene, DragonScene, MultipleObjectsScene, GlassScene

Application & Application::getInstance() {
	static Application application;
	return application;
}

void Application::init() {
	std::cout << "Initialization started." << std::endl;

	// -------------------------------------
	// Initialize GLFW.
	// -------------------------------------
	if (!glfwInit()) {
		std::cerr << "GLFW failed to initialize." << std::endl;
	}
	double timeElapsed = glfwGetTime();

	// OpenGL version 4.5 with core profile.
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE); // glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
	glfwWindowHint(GLFW_SAMPLES, MSAA_SAMPLES);

	// Create a new window and set window mode accordingly (including title, size and mode).
#if DEFAULT_FULLSCREEN == 2 
	// Borderless fullscreen.
	SetBorderlessFullscreenMode();
#else
	SetWindowMode(DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT, DEFAULT_FULLSCREEN == 1);
#endif
	std::cout << "[0] : GLFW initialized." << std::endl;

	// -------------------------------------
	// Initialize GLEW.
	// -------------------------------------
	glewExperimental = GL_TRUE;
	if (glewInit() == GLEW_OK) {
		std::cout << "[1] : GLEW initialized." << std::endl;
	}
	else {
		std::cerr << "GLEW failed to initialize (glewExperimental might not be supported)." << std::endl;
		std::cerr << "Press any key to exit ... " << std::endl;
		getchar();
		return;
	}
/*
	std::vector<std::pair<GLuint, std::string>> requiredGLEWExtensions = {
		{ GLEW_ARB_shader_image_load_store,		"ARB_shader_image_load_store"},
		{ GLEW_VERSION_4_5,						"GLEW_VERSION_4_5 (OpenGL 4.5)"},
		{ GL_ARB_multisample,					"GLFW MSAA" }
	};

	for (const auto & ext : requiredGLEWExtensions) {
		if (!ext.first) {
			std::cerr << "ERROR: " << ext.second << " not supported! Expect unexpected behaviour." << std::endl;
			std::cerr << "Press any key to continue ... " << std::endl;
			getchar();
		}
	}
*/

	// -------------------------------------
	// Initialize graphics.
	// -------------------------------------
	int w, h;
	glfwGetWindowSize(currentWindow, &w, &h);
	MaterialStore::getInstance(); // Initialize material store.
	graphics.init(w, h);
	glfwSetWindowSizeCallback(currentWindow, Application::OnWindowResize);
	glfwSwapInterval(DEFAULT_VSYNC); // vSync.
	std::cout << "[2] : Graphics initialized." << std::endl;

	// -------------------------------------
	// Initialize scene.
	// -------------------------------------
	scene = new __DEFAULT_LEVEL();
	scene->init(w, h);
	std::cout << "[3] : Scene initialized." << std::endl;

	// Point lights.
	if (scene->pointLights.size() > 0) {
		int pp = 0;
		for (auto & p : scene->pointLights) {
			if (!p.tweakable) continue;
			std::string group = "Point Light " + std::to_string(pp);
			std::string setting = "group='" + group + "'";
			pp++;
		}
	}

	// -------------------------------------
	// Initialize input.
	// -------------------------------------
	glfwSetInputMode(currentWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwSetMouseButtonCallback(currentWindow, GLFWMouseButtonCallback);
	glfwSetCursorPosCallback(currentWindow, GLFWMousePositionCallback);
	glfwSetKeyCallback(currentWindow, GLFWKeyCallback);
	std::cout << "[5] : Input initialized." << std::endl;

	// -------------------------------------
	// Finalize initialization.
	// -------------------------------------
	srand(0);
	Time::time = 0;
	initialized = Time::initialized = true;

	timeElapsed = glfwGetTime() - timeElapsed;
	std::cout << "Initialization finished (" << timeElapsed << " seconds)!" << std::endl;
	glfwSetTime(0);

	std::cout << "Using OpenGL version " << glGetString(GL_VERSION) << std::endl;
}

void Application::run()
{
	std::cout << "Application is now running.\n" << std::endl;
	std::cout << " :: Use R to switch between rendering modes.\n";
	// std::cout << " :: Use T to switch between interaction modes." << std::endl;

	double smoothedDeltaTimeAccumulator = 0;
#if __LOG_INTERVAL > 0
	double updateCost = 0;
	double renderCost = 0;
	double timestampCost = 0;
	double timestampLog = 0;
#endif

	// Start the update loop.
	while (!glfwWindowShouldClose(currentWindow) && !exitQueued)
	{
		// --------------------------------------------------
		// Update input and timers.
		// --------------------------------------------------
		UpdateGlobalInputParameters();

		// Update time vals.
		double currentTime = glfwGetTime();
		Time::deltaTime = currentTime - Time::time;
		Time::time = currentTime;
		Time::framesPerSecond = 1.0 / (Time::deltaTime + 0.0000001);

		// --------------------------------------------------
		// Update world.
		// --------------------------------------------------
#if __LOG_INTERVAL > 0
		{
			timestampCost = glfwGetTime();
		}
#endif
		if (!paused) scene->update();
#if __LOG_INTERVAL > 0 
		{
			updateCost += glfwGetTime() - timestampCost;
		}
#endif

		// --------------------------------------------------
		// Rendering.
		// --------------------------------------------------
#if __LOG_INTERVAL > 0
		{
			timestampCost = glfwGetTime();
		}
#endif
		int viewportWidth, viewportHeight;
		glfwGetWindowSize(currentWindow, &viewportWidth, &viewportHeight);
		if (!paused) {
			graphics.render(*scene, viewportWidth, viewportHeight, currentRenderingMode);

		}

		// --------------------------------------------------
		// Swap buffers and update timers.
		// --------------------------------------------------
#if __LOG_INTERVAL > 0
		{
			renderCost += glfwGetTime() - timestampCost;
		}
#endif

		// Swap front and back buffers.
		if (!paused) glfwSwapBuffers(currentWindow);

		// Poll for and process events.
		glfwPollEvents();

		// Update frame count.
		Time::frameCount++;
		smoothedDeltaTimeAccumulator += Time::deltaTime;

		// Update smoothed delta time.
		if (Time::smoothedDeltaTimeFrameCount > 0 && Time::frameCount % Time::smoothedDeltaTimeFrameCount == 0) {
			Time::smoothedDeltaTime = smoothedDeltaTimeAccumulator / Time::smoothedDeltaTimeFrameCount;
			smoothedDeltaTimeAccumulator = 0;
		}

#if __LOG_INTERVAL > 0
		{
			using namespace std;
			timestampLog += Time::deltaTime;
			if (Time::frameCount % __LOG_INTERVAL == 0 && timestampLog > __LOG_INTERVAL_TIME_GUARD) {
				cout << endl;
				cout << setprecision(8) << fixed << "State: " << state << ", update average time: " << updateCost / __LOG_INTERVAL << ", render average time: " << renderCost / __LOG_INTERVAL << endl;
				cout << setprecision(1) << fixed << "FPS: " << Time::framesPerSecond << " smoothed " << 1.0f / Time::smoothedDeltaTime << setprecision(4) << ", delta: " << Time::deltaTime << ", frame count: " << Time::frameCount << ", smooth delta: " << Time::smoothedDeltaTime << endl;
				updateCost = 0;
				renderCost = 0;
				cout << flush;
				timestampLog = 0;
			}
		}
#endif
	}

	// Clean up and exit.
	glfwDestroyWindow(currentWindow);
	glfwTerminate();
	// TwTerminate();
	std::cout << "Application has now terminated." << std::endl;
}

void Application::UpdateGlobalInputParameters() {
	if (glfwGetKey(currentWindow, GLFW_KEY_ESCAPE)) {
		exitQueued = true;
		return;
	}

	int button_state_x = glfwGetKey(currentWindow, GLFW_KEY_X);
	int button_state_z = glfwGetKey(currentWindow, GLFW_KEY_Z);

	if (button_state_x == GLFW_PRESS && button_state_x != previous_state_x) {
		std::cout << "Application state: " << ++state << std::endl;
	}
	if (button_state_z == GLFW_PRESS && button_state_z != previous_state_z) {
		std::cout << "Application state: " << --state << std::endl;
	}

	previous_state_x = button_state_x;
	previous_state_z = button_state_z;
}

void Application::SetBorderlessFullscreenMode() {
	GLFWmonitor* monitor = glfwGetPrimaryMonitor();
	const GLFWvidmode* mode = glfwGetVideoMode(monitor);
	glfwWindowHint(GLFW_RED_BITS, mode->redBits);
	glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
	glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
	glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);
	GLFWwindow* newWindow = glfwCreateWindow(mode->width, mode->height, DEFAULT_TITLE, monitor, NULL);
	if (newWindow) {
		currentWindow = newWindow;
		glfwMakeContextCurrent(currentWindow);
	}
	else {
		glfwTerminate();
		std::cerr << "GLFW failed setting borderless fullscreen window mode." << std::endl;
	}
}

void Application::SetWindowMode(int quadWidth, int quadHeight, bool fullscreen = true) {
	GLFWwindow* newWindow = glfwCreateWindow(quadWidth, quadHeight, DEFAULT_TITLE, fullscreen ? glfwGetPrimaryMonitor() : NULL, NULL);
	if (newWindow) {
		currentWindow = newWindow;
		glfwMakeContextCurrent(currentWindow);
	}
	else {
		glfwTerminate();
		std::cerr << "GLFW failed setting new window mode." << std::endl;
	}
}

Application::~Application() {
	delete scene;
}

Application::Application() : exitQueued(false) {

}

void Application::OnWindowResize(GLFWwindow* window, int quadWidth, int quadHeight)
{

}

void Application::GLFWMousePositionCallback(GLFWwindow * window, double x, double y)
{

}

void Application::GLFWMouseButtonCallback(GLFWwindow * window, int button, int action, int mods)
{
	
}

void Application::GLFWKeyCallback(GLFWwindow * window, int key, int scanmode, int action, int mods)
{
	auto & app = Application::getInstance();


	// Button was pressed down this frame.
	if (action == 1) {
		// Change rendering mode.
		if (key == GLFW_KEY_R) {
			using GRM = Graphics::RenderingMode;
			if (app.currentRenderingMode == GRM::VOXELIZATION_VISUALIZATION) {
				app.currentRenderingMode = GRM::VOXEL_CONE_TRACING;
			}
			else {
				app.currentRenderingMode = GRM::VOXELIZATION_VISUALIZATION;
			}
		}

		// Change viewing mode.
		if (key == GLFW_KEY_M) {
			if (app.currentInputState == InputState::FREE_LOOK) {
				glfwSetInputMode(app.currentWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
				app.currentInputState = InputState::TWEAK_BAR;
			}
			else {
				app.currentInputState = InputState::FREE_LOOK;
				glfwSetInputMode(app.currentWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			}
			// Update camera.
			int xwidth, yheight;
			glfwGetWindowSize(window, &xwidth, &yheight);
			glfwSetCursorPos(window, xwidth / 2, yheight / 2); // Reset mouse position for next update iteration.
		}

		// Pause / unpause.
		if (key == GLFW_KEY_P) {
			app.paused = !app.paused;
		}
	}
}
