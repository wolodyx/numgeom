#version 450

// Выходные переменные
layout(location = 0) out vec3 fragColor;

// Координаты треугольника и цвета (в NDC пространстве)
vec2 positions[3] = vec2[](
    vec2(0.0, -0.5),  // Верхняя вершина
    vec2(0.5, 0.5),   // Правая нижняя
    vec2(-0.5, 0.5)   // Левая нижняя
);

vec3 colors[3] = vec3[](
    vec3(1.0, 0.0, 0.0), // Красный
    vec3(0.0, 1.0, 0.0), // Зеленый
    vec3(0.0, 0.0, 1.0)  // Синий
);

void main() {
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
    fragColor = colors[gl_VertexIndex];
}
