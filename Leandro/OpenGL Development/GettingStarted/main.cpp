#include <GL\glew.h>
#include <GL\freeglut.h>
#include <iostream>
#include "GLSLShader.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#pragma comment(lib, "glew32.lib")

using namespace std;

struct Vertex
{
	glm::vec3 color;
	glm::vec3 position;
};


const int WIDTH = 800;
const int HEIGTH = 600;
GLSLShader shader;
Vertex vertices[3];
GLushort indices[3];

//projection and modelview matrices
glm::mat4  P = glm::mat4(1);
glm::mat4 MV = glm::mat4(1);

//vertex array and vertex buffer object IDs
GLuint vaoID;
GLuint vboVerticesID;
GLuint vboIndicesID;

void OnInit()
{
	
	//load the shader
	shader.LoadFromFile(GL_VERTEX_SHADER, "shader.vert");
	shader.LoadFromFile(GL_FRAGMENT_SHADER, "shader.frag");
	//compile and link shader
	shader.CreateAndLinkProgram();
	shader.Use();
	//add attributes and uniforms
	shader.AddAttribute("vVertex");
	shader.AddAttribute("vColor");
	shader.AddUniform("MVP");
	shader.UnUse();

	

	//setup triangle geometry
	//setup triangle vertices
	vertices[0].color = glm::vec3(1, 0, 0);
	vertices[1].color = glm::vec3(0, 1, 0);
	vertices[2].color = glm::vec3(0, 0, 1);

	vertices[0].position = glm::vec3(-1, -1, 0);
	vertices[1].position = glm::vec3(0, 1, 0);
	vertices[2].position = glm::vec3(1, -1, 0);

	//setup triangle indices
	indices[0] = 0;
	indices[1] = 1;
	indices[2] = 2;

	//setup triangle vao and vbo stuff
	glGenVertexArrays(1, &vaoID);
	glGenBuffers(1, &vboVerticesID);
	glGenBuffers(1, &vboIndicesID);

	glBindVertexArray(vaoID);

	glBindBuffer(GL_ARRAY_BUFFER, vboVerticesID);
	//pass triangle verteices to buffer object
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), &vertices[0], GL_STATIC_DRAW);
	
	//enable vertex attribute array for position
	glEnableVertexAttribArray(shader["vVertex"]);
	glVertexAttribPointer(shader["vVertex"], 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const GLvoid*)offsetof(Vertex, position));
	
	//enable vertex attribute array for colour
	glEnableVertexAttribArray(shader["vColor"]);
	glVertexAttribPointer(shader["vColor"], 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const GLvoid*)offsetof(Vertex, color));
	
	//pass indices to element array buffer
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vboIndicesID);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), &indices[0], GL_STATIC_DRAW);
	
	cout << "Initialization successfull" << endl;
}

void OnShutdown()
{
	//Destroy shader
	shader.DeleteShaderProgram();

	//Destroy vao and vbo
	glDeleteBuffers(1, &vboVerticesID);
	glDeleteBuffers(1, &vboIndicesID);
	glDeleteVertexArrays(1, &vaoID);

	cout << "Shutdown successfull" << endl;
}

void OnResize(int w, int h)
{
	//set the viewport size
	glViewport(0, 0, (GLsizei)w, (GLsizei)h);
	//setup the projection matrix
	P = glm::ortho(-1, 1, -1, 1);
}

void OnRender()
{
	//clear the colour and depth buffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//bind the shader
	shader.Use();
	//pass the shader uniform
	glUniformMatrix4fv(shader("MVP"), 1, GL_FALSE, glm::value_ptr(P*MV));
	//drwa triangle
	glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_SHORT, 0);
	//unbind the shader
	shader.UnUse();

	//swap front and back buffers to show the rendered result
	glutSwapBuffers();
}

void StartGlew()
{
	glewExperimental = GL_TRUE;
	GLenum err = glewInit();
	if (GLEW_OK != err)
	{
		cerr << "Error: " << glewGetErrorString(err) << endl;
	}
	else
	{
		if (GLEW_VERSION_3_3)
		{
			cout << "Driver Suports OpenGL 3.3\nDetails: " << endl;
		}
	}
	cout << "\tUsing glew " << glewGetString(GLEW_VERSION) << endl;
	cout << "\tVendor: " << glGetString(GL_VENDOR) << endl;
	cout << "\tRenderer: " << glGetString(GL_RENDERER) << endl;
	cout << "\tVersion: " << glGetString(GL_VERSION) << endl;
}

int main(int argc, char** argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
	glutInitContextVersion(3, 3);
	glutInitContextFlags(GLUT_CORE_PROFILE | GLUT_DEBUG);
	glutInitContextProfile(GLUT_FORWARD_COMPATIBLE);
	glutInitWindowSize(WIDTH, HEIGTH);
	glutCreateWindow("Getting Started with OpenGL 3.3");
	StartGlew();
	OnInit();
	glutCloseFunc(OnShutdown);
	glutDisplayFunc(OnRender);
	glutReshapeFunc(OnResize);
	glutMainLoop();
	return 0;
}