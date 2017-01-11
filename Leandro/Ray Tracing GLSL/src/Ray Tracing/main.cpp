#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <iostream>
#include <math.h>

#include <GL/glew.h>
#include <GL/glut.h>

#include "textfile.h"

//#define M_PI 3.1415926535897932384626433832795

// Controls shading model
GLuint program;
GLint loc;

// Uniforms
float width = 800;
float height = 400;
float d = 0.1;
float addX = 0;
float addY = 0;
float addZ = 0;
float addBallX = 0;
float addBallY = 0;
float addBallZ = 0;
int samplesAA = 0;
int maxSamples = 32;
float theta = 0;
float fi = 0;
float r = 1;

void UpdateUniforms();
void SetShaders();
void InstallShaders(const GLchar *phongVertex, const GLchar *phongFragment);
void log();
void calcXYZ();


// Region OpenGLCallbacks

void init(void)
{
    glClearColor(0.5, 0.5, 0.5, 0.0);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glewInit();
    SetShaders();
}

void idle()
{
    calcXYZ();
    if(samplesAA < maxSamples)
    {
        samplesAA++;
        system("cls");
        std::cout << "Samples: " << samplesAA << std::endl;
        std::cout << "X: " << addBallX << std::endl;
        std::cout << "Y: " << addBallY << std::endl;
        std::cout << "Z: " << addBallZ << std::endl;
        std::cout << "Theta: " << theta << std::endl;
        std::cout << "Fi: " << fi << std::endl;
    }
    width = glutGet(GLUT_WINDOW_WIDTH);
    height = glutGet(GLUT_WINDOW_HEIGHT);
    UpdateUniforms();
    glutPostRedisplay();
}

void display(void)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glLoadIdentity();
	gluLookAt(0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0f, 1.0f, 0.0f);

	glPushMatrix();
        glBegin(GL_QUADS);
            glVertex2f(-1.0, -1.0);
            glVertex2f(-1.0, 1.0);
            glVertex2f(1.0, 1.0);
            glVertex2f(1.0, -1.0);
        glEnd();
	glPopMatrix();

    glutSwapBuffers();

    log();
}

void reshape(int w, int h)
{
	// Prevent a divide by zero, when window is too short
	// (you cant make a window of zero width).
    if(h == 0)
    {
        h = 1;
    }

	float ratio = 1.0* w / h;

	// Reset the coordinate system before modifying
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	// Set the viewport to be the entire window
    glViewport(0, 0, w, h);

	// Set the correct perspective.
	gluPerspective(45,ratio,1,1000);
	glMatrixMode(GL_MODELVIEW);
}



// Region ControlCallbacks

void special(int key, int x, int y)
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


void keyboard(unsigned char key, int x, int y)
{
    switch(tolower(key))
    {
        case 'a':
            addX -= 0.1;
        break;
        case 'd':
            addX += 0.1;
        break;
        case 'w':
            addY += 0.1;
        break;
        case 's':
            addY -= 0.1;
        break;
        case 'q':
            addZ += 0.1;
        break;
        case 'e':
            addZ -= 0.1;
        break;
    }
}


void motion(int x, int y )
{

}


void mouse(int button, int state, int x, int y)
{

}



// Region Functions

void UpdateUniforms()
{
    loc = glGetUniformLocation(program, "addX");
	glUniform1f(loc, addX);
	loc = glGetUniformLocation(program, "addY");
	glUniform1f(loc, addY);
	loc = glGetUniformLocation(program, "addZ");
	glUniform1f(loc, addZ);
	loc = glGetUniformLocation(program, "addBallX");
	glUniform1f(loc, addBallX);
	loc = glGetUniformLocation(program, "addBallY");
	glUniform1f(loc, addBallY);
	loc = glGetUniformLocation(program, "addBallZ");
	glUniform1f(loc, addBallZ);
	loc = glGetUniformLocation(program, "width");
	glUniform1f(loc, width);
	loc = glGetUniformLocation(program, "height");
	glUniform1f(loc, height);
	loc = glGetUniformLocation(program, "samplesAA");
	glUniform1i(loc, samplesAA);
}

void InstallShaders(const GLchar *phongVertex, const GLchar *phongFragment)
{
	GLuint phongVS;
	GLuint phongFS;

	// Create a vertex shader object and a fragment shader object
	phongVS = glCreateShader(GL_VERTEX_SHADER);
	phongFS = glCreateShader(GL_FRAGMENT_SHADER);

	// Load source code strings into shaders
	glShaderSource(phongVS, 1, &phongVertex, NULL);
	glShaderSource(phongFS, 1, &phongFragment, NULL);

	// Compile the phong vertex shader and print out
	// the compiler log file.
	glCompileShader(phongVS);
	glCompileShader(phongFS);

	program = glCreateProgram();
	glAttachShader(program, phongVS);
	glAttachShader(program, phongFS);
	glLinkProgram(program);

	// Install program object as part of current state
	glUseProgram(program);
}

void SetShaders()
{
	char *vs = NULL;
	char *fs = NULL;

    glCreateShader(GL_VERTEX_SHADER);
    glCreateShader(GL_FRAGMENT_SHADER);

	vs = textFileRead((char*) "../src/Ray Tracing/shaders/shader.vert");
	fs = textFileRead((char*) "../src/Ray Tracing/shaders/shader.frag");

	if(!vs)
	{
		printf("File \"rt.vert\" not found.\n");
		exit(1);
	}

	if(!fs)
	{
		printf("File \"rt.frag\" not found.\n");
		exit(1);
	}

	const char * ff = fs;
	const char * vv = vs;

	InstallShaders(vv, ff);
}

void log()
{

}

void calcXYZ()
{
    addBallZ = r*sin(theta)*cos(fi);
    addBallX = r*sin(theta)*sin(fi);
    addBallY = r*cos(theta);
}


int main(int argc, char** argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(width, height);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("Ray Tracing");
    init();
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(special);
    glutIdleFunc(idle);
    glutMainLoop();
    return 0;
}
