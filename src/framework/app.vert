#version 420 core

layout(std140, set = 0, binding = 0) uniform block {
    uniform mat4 mvpMatrix;
};

layout(location = 0) in vec4 in_position;

vec4 lightSource = vec4(2.0, 2.0, 20.0, 0.0);

layout(location = 0) out vec4 vVaryingColor;

// Функция для преобразования HSV в RGB
vec3 hsv2rgb(vec3 c) {
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

void main()
{
    gl_Position = mvpMatrix * in_position;

    int vertexID = gl_VertexIndex;
    // Создаем псевдослучайный цвет на основе ID
    float hue = fract(float(vertexID) * 0.618033988749895); // Золотое сечение
    float saturation = 0.8;
    float value = 0.9;

    vVaryingColor = vec4(hsv2rgb(vec3(hue, saturation, value)),1.0);
}
