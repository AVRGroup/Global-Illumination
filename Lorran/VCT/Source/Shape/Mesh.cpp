#include "Mesh.h"

#define GLEW_STATIC
#include "../Include/GL/glew.h"
#include "../Include/GLFW/glfw3.h"
#include "../Include/glm/gtc/type_ptr.hpp"

Mesh::Mesh() { }

Mesh::~Mesh() {
	if (meshUploaded) {
		GLint curp;
		glGetIntegerv(GL_CURRENT_PROGRAM, &curp);

		// Delete.
		glUseProgram(program);
		glDeleteBuffers(1, &vbo);
		glDeleteVertexArrays(1, &vao);

		// Reset.
		glUseProgram(curp);
	}
}
