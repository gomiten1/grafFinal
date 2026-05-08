#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D texture_diffuse1;
uniform float time;
uniform vec3 waterColorDark;
uniform vec3 waterColorLight;

void main()
{   
    vec2 coordsT = TexCoords;
    coordsT.x += 0.018f * time;
    coordsT.y += 0.018f * time;

    // Crear patrón de onda procedural (velocidad de desplazamiento y fase reducidas)
    float wave = sin(coordsT.x * 10.0) * 0.5 + 0.5;
    wave += sin(coordsT.y * 10.0 + time * 0.22f) * 0.3;
    wave += sin((coordsT.x + coordsT.y) * 5.0 - time * 0.22f) * 0.2;
    
    // Color del agua configurable desde C++ según sceneMode
    vec3 waterColor = mix(
        waterColorDark,
        waterColorLight,
        wave
    );
    
    FragColor = vec4(waterColor, 0.85);
}