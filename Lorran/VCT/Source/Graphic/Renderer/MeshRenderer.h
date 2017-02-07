#pragma once

#include "../../Shape/Transform.h"
#include "../Material/MaterialSetting.h"

#define GLEW_STATIC
#include "../Include/GL/glew.h"
#include "../Include/GLFW/glfw3.h"
#include "../Include/glm/glm.hpp"
#include "../Include/glm/gtc/type_ptr.hpp"

class Mesh;

/// <summary> A renderer that can be used to render a mesh. </summary>
class MeshRenderer {
public:
	bool enabled = true;
	bool tweakable = false; // Automatically adds a window for this mesh renderer.
	std::string name = "Mesh renderer"; // Is displayed in the tweak bar.

	Transform transform;
	Mesh * mesh;

	// Constr/destr.
	MeshRenderer(Mesh *, MaterialSetting * = nullptr);
	~MeshRenderer();

	// Rendering.
	MaterialSetting * materialSetting = nullptr;
	void render(const GLuint program);
private:
	void setupMeshRenderer();
	void reuploadIndexDataToGPU();
	void reuploadVertexDataToGPU();
};