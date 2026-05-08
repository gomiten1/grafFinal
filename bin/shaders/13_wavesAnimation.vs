#version 330 core
layout (location = 0) in vec3  aPos;
layout (location = 1) in vec3  aNormal;
layout (location = 2) in vec2  aTexCoords;
layout (location = 3) in vec3  tangent;
layout (location = 4) in vec3  bitangent;

out vec2 TexCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform float time;
uniform float radius;
uniform float height;

void main()
{
    
    vec4 PosL = vec4(aPos, 1.0f);
    // height controla amplitud (antes ~0.02 con height=5); time * factor = velocidad de fase
    float amp = height * 0.00004f;
    float phaseSpeed = 0.22f;
    PosL.z += amp * sin(PosL.x * 2.0f + time * phaseSpeed);
    PosL.z += amp * sin(PosL.y * 2.0f + time * phaseSpeed);

    gl_Position = projection * view * model * PosL;

    TexCoords = aTexCoords;  
}