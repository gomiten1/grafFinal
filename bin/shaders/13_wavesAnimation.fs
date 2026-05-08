#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D texture_diffuse1;
uniform float time;

void main()
{   
    vec2 coordsT = TexCoords;
    coordsT.x += 0.018f * time;
    coordsT.y += 0.018f * time;

    // Crear patrón de onda procedural (velocidad de desplazamiento y fase reducidas)
    float wave = sin(coordsT.x * 10.0) * 0.5 + 0.5;
    wave += sin(coordsT.y * 10.0 + time * 0.22f) * 0.3;
    wave += sin((coordsT.x + coordsT.y) * 5.0 - time * 0.22f) * 0.2;
    
    // Color azul con variación por el patrón
    vec3 waterColor = mix(
        vec3(0.1, 0.4, 0.8),  // Azul oscuro
        vec3(0.3, 0.7, 1.0),  // Azul claro
        wave
    );
    
    FragColor = vec4(waterColor, 0.85);
}