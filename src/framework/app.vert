#version 420 core

layout(std140, set = 0, binding = 0) uniform block {
    uniform mat4 mvpMatrix;
};

layout(location = 0) in vec4 in_position;

vec4 lightSource = vec4(2.0, 2.0, 20.0, 0.0);

layout(location = 0) out vec4 vVaryingColor;

void main()
{
    gl_Position = mvpMatrix * in_position;
    vVaryingColor = vec4(0.5, 0.3, 0.1, 1.0);
}
