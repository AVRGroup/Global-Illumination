#version 330 core

// VERTEX SHADER

//object space vertex position
layout(location = 0) in vec2 vVertex;

//uniform
//combined modelview projection matrix
uniform mat4 MVP;

void main()
{
   //get the clip space position by multiplying the combined MVP matrix with the object space
   //vertex position
   gl_Position = MVP*vec4(vVertex,1,1);
}
