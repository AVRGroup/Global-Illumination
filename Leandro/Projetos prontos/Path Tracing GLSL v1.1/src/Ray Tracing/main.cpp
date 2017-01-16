#include <GL/glew.h>
#include <GL/freeglut.h>
#include <iostream>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "GLSLShader.h"

using namespace std;

//screen size
int WIDTH = 800;
int HEIGHT = 400;

float d = 0.1;
float addBallX = 0;
float addBallY = 0;
float addBallZ = 0;
int samplesAA = 0;
int maxSamples = 32;
float theta = 1.5;
float fi = 5.5;
float r = 3;

//shader reference
GLSLShader shader;

//vertex array and vertex buffer object for fullscreen quad
GLuint vaoID;
GLuint vboVerticesID;
GLuint vboIndicesID;

//quad vertices and indices
glm::vec2 vertices[4];
GLushort indices[6];

glm::mat4  P = glm::mat4(1);
glm::mat4 MV = glm::mat4(1);


void UpdateUniforms();
void SetShaders();
void SetGeometry();
void CalcXYZ();
void GlewInit();


//OpenGL initialization
void OnInit()
{
	SetShaders();
	UpdateUniforms();
	SetGeometry();

	cout << "Initialization successfull" << endl;
	cout << "------------------------------------------------------" << endl;
	cout << "Controles" << endl;
	cout << "------------------------------------------------------" << endl;
	cout << "Rotacionar camera: setas" << endl;
	cout << "Zoom camera: PgUp e PgDn" << endl;
}


void OnIdle()
{
    CalcXYZ();
    if(samplesAA < maxSamples)
    {
        samplesAA++;
    }
    WIDTH = glutGet(GLUT_WINDOW_WIDTH);
    HEIGHT = glutGet(GLUT_WINDOW_HEIGHT);

    UpdateUniforms();
    glutPostRedisplay();
}


//resize event handler
void OnResize(int w, int h)
{
	//set the viewport
	glViewport(0, 0, (GLsizei)w, (GLsizei)h);
}


//display function
void OnRender()
{
	//clear the colour and depth buffers
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//bind shader
	shader.Use();

	glUniformMatrix4fv(shader("MVP"), 1, GL_FALSE, glm::value_ptr(P*MV));

	//draw the full screen quad
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);

	//unbind shader
	shader.UnUse();

	//swap front and back buffers to show the rendered result
	glutSwapBuffers();
}


//release all allocated resources
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


void OnSpecialDown(int key, int x, int y)
{
    switch (key)
    {
        case GLUT_KEY_LEFT:
            samplesAA = 0;
            fi -= d;
        break;
        case GLUT_KEY_RIGHT:
            samplesAA = 0;
            fi += d;
        break;
        case GLUT_KEY_UP:
            samplesAA = 0;
            theta -= d;
        break;
        case GLUT_KEY_DOWN:
            samplesAA = 0;
            theta += d;
        break;
        case GLUT_KEY_PAGE_DOWN:
            samplesAA = 0;
            r += d;
        break;
        case GLUT_KEY_PAGE_UP:
            samplesAA = 0;
            r -= d;
        break;
    }
}


void SetShaders()
{
    //load shader
	shader.LoadFromFile(GL_VERTEX_SHADER, "../src/Ray Tracing/shaders/shader.vert");
	shader.LoadFromFile(GL_FRAGMENT_SHADER, "../src/Ray Tracing/shaders/shader.frag");
	//compile and link shader
	shader.CreateAndLinkProgram();
	shader.Use();
	//add attributes and uniforms
	shader.AddAttribute("vVertex");
	shader.AddUniform("MVP");
	shader.AddUniform("width");
	shader.AddUniform("height");
	shader.AddUniform("addBallX");
	shader.AddUniform("addBallY");
	shader.AddUniform("addBallZ");
	shader.AddUniform("samplesAA");
	//pass values of constant uniforms at initialization
	shader.UnUse();
}


void UpdateUniforms()
{
	shader.Use();
	glUniform1f(shader("width"), WIDTH);
	glUniform1f(shader("height"), HEIGHT);
	glUniform1f(shader("addBallX"), addBallX);
	glUniform1f(shader("addBallY"), addBallY);
	glUniform1f(shader("addBallZ"), addBallZ);
	glUniform1i(shader("samplesAA"), samplesAA);
	shader.UnUse();
}


void SetGeometry()
{
    //setup quad geometry
	//setup quad vertices
	vertices[0] = glm::vec2(-1.0, -1.0);
	vertices[1] = glm::vec2(1.0, -1.0);
	vertices[2] = glm::vec2(1.0, 1.0);
	vertices[3] = glm::vec2(-1.0, 1.0);
	//fill quad indices array
	GLushort* id = &indices[0];
	*id++ = 0;
	*id++ = 1;
	*id++ = 2;
	*id++ = 0;
	*id++ = 2;
	*id++ = 3;

	//setup quad vao and vbo stuff
	glGenVertexArrays(1, &vaoID);
	glGenBuffers(1, &vboVerticesID);
	glGenBuffers(1, &vboIndicesID);
	glBindVertexArray(vaoID);
	glBindBuffer(GL_ARRAY_BUFFER, vboVerticesID);
	//pass quad vertices to buffer object
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), &vertices[0], GL_STATIC_DRAW);
	//enable vertex attribute array for position
	glEnableVertexAttribArray(shader["vVertex"]);
	glVertexAttribPointer(shader["vVertex"], 2, GL_FLOAT, GL_FALSE, 0, 0);
	//pass quad indices to element array buffer
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vboIndicesID);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), &indices[0], GL_STATIC_DRAW);
}


void CalcXYZ()
{
    addBallZ = r*sin(theta)*cos(fi);
    addBallX = r*sin(theta)*sin(fi);
    addBallY = r*cos(theta);
}


void GlewInit()
{
    //glew initialization
	glewExperimental = GL_TRUE;
	GLenum err = glewInit();
	if (GLEW_OK != err)
    {
		cerr << "Error: " << glewGetErrorString(err) << endl;
	}
	else if (GLEW_VERSION_3_3)
    {
        cout << "Driver supports OpenGL 3.3\nDetails:" << endl;
    }

	err = glGetError(); //this is to ignore INVALID ENUM error 1282

    //print information on screen
	cout << "\tUsing GLEW " << glewGetString(GLEW_VERSION) << endl;
	cout << "\tVendor: " << glGetString(GL_VENDOR) << endl;
	cout << "\tRenderer: " << glGetString(GL_RENDERER) << endl;
	cout << "\tVersion: " << glGetString(GL_VERSION) << endl;
	cout << "\tGLSL: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;
}

int main(int argc, char** argv)
{
	//freeglut initialization calls
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
	glutInitContextVersion(3, 3);
	glutInitContextFlags(GLUT_CORE_PROFILE | GLUT_DEBUG);
	glutInitWindowSize(WIDTH, HEIGHT);
	glutCreateWindow("Path Tracer - OpenGL 3.3");

	GlewInit();

	//initialization of OpenGL
	OnInit();

	//callback hooks
	glutCloseFunc(OnShutdown);
	glutDisplayFunc(OnRender);
	glutReshapeFunc(OnResize);
	glutIdleFunc(OnIdle);
	glutSpecialFunc(OnSpecialDown);

	//main loop call
	glutMainLoop();

	return 0;
}
