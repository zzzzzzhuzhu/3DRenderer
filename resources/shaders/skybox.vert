#version 330 core

// Shader used for the skybox

layout (location = 0) in vec3 aPos;

out vec3 TexCoords;

uniform mat4 VP;

void main()
{
    TexCoords = aPos;
    gl_Position = VP * vec4(aPos, 1.0);
}
