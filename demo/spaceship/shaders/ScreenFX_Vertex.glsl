#version 330 core
layout (location = 0) in vec2 position;
layout (location = 1) in vec4 in_color;
layout (location = 2) in vec2 texCoords;
//layout (location = 3) in vec3 in_normal;
out vec2 TexCoords;

void main()
{
    gl_Position = vec4(position.x, position.y, 0.0f, 1.0f); 
    TexCoords = texCoords;
}
