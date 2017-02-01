#include "Mesh.h"

#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/type_ptr.hpp>

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
