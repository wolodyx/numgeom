#version 450

// Входные переменные (должны соответствовать вершинному шейдеру)
layout(location = 0) in vec3 fragColor;

// Выходной цвет
layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(fragColor, 1.0); // RGB + альфа
}
