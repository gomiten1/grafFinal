# Proyecto: Práctias OpenGL

Conjunto de ejercicios y ejemplos de laboratorio para prácticas con OpenGL, shaders, carga de modelos y animación.

## Contenido principal
- Código de ejemplos: `15_Final/` (ejemplo de animación y carga de muchos modelos).
- Ejecutables y recursos esperados en: `project1/bin/` y `bin/`.
- Modelos: `models/` (varios .fbx/.fbm usados por los ejemplos).
- Shaders: `shaders/` (múltiples archivos GLSL usados por los ejemplos).
- Texturas: `textures/` (incluye cubemaps y imágenes auxiliares).
- Dependencias locales: `deps/` (glad, glfw, glm, assimp, irrklang).

## Descripción
Este repositorio contiene ejercicios y un ejemplo final (`15_Final`) que muestra:
- Carga y renderizado de múltiples modelos 3D (Assimp).
- Animaciones por esqueleto (skinning) y shaders para efectos (Phong, Fresnel, procedurales).
- Manejo de cámaras, luces, cubemap y reproducción de audio con irrKlang (opcional).

## Requisitos 
- Compilador: `g++` (C++17).
- Herramientas: `pkg-config`.
- Librerías del sistema: `libglfw3-dev`, `libassimp-dev`, OpenGL headers/lib (`libgl1-mesa-dev` o similar).
- El script de construcción usa dependencias locales en `deps/` (glad, glm, glfw, assimp). Si faltan, instálelas o adapte el script.

## Cómo compilar y ejecutar
1. Abrir una terminal y posicionarse en la carpeta del ejercicio (por ejemplo `15_Final/`):

   cd 15_Final

2. Usar el script de la raíz para compilar y ejecutar desde cualquier ejercicio:

   ../build_and_run.sh

Opciones útiles:
- `--build-only` : solo compila, no ejecuta.
- `--name NOMBRE` : especifica el nombre del ejecutable en `project1/bin/`.

Ejemplo para compilar sin ejecutar:

   cd 15_Final
   ../build_and_run.sh --build-only

## Notas importantes
- El script `build_and_run.sh` busca headers y fuentes en `deps/` y requiere que `pkg-config` encuentre `glfw3` y `assimp` en el sistema o que los includes/libs estén en `deps/`.
- Si no se dispone de `irrklang` en Linux, el script crea un stub (sin audio) para permitir la compilación.
- El script incorpora `deps/glad/MSVC2022/src/glad.c` al enlazado; verifique que la ruta exista o adapte el script.
- Rutas relativas a shaders, modelos y texturas se resuelven desde `project1/bin/` tras la compilación (el script crea enlaces simbólicos cuando procede).

## Estructura relevante
- [15_Final/](15_Final) : ejemplo principal de animación y escena con múltiples modelos.
- [build_and_run.sh](build_and_run.sh) : script para compilar y ejecutar ejercicios desde la raíz.
- [deps/](deps) : dependencias empaquetadas (GLAD, GLFW, GLM, Assimp, etc.).
- [models/](models), [shaders/](shaders), [textures/](textures) : recursos usados por los ejemplos.

## Solución de problemas rápida
- Error "no se encontró glfw3": instale `libglfw3-dev` o configure `pkg-config`.
- Error "no se encontró Assimp": instale `libassimp-dev`.
- Si falta `glad.c` en `deps/glad/.../src/`, copie la implementación de GLAD o genere una nueva desde glad.dav1d.de.

## Créditos
- Código y estructura: material de laboratorio/curso de Computación Gráfica e IHC.
- Sebastián Ospino
- Emilio Arriaga
- Donnovan
- Recursos 3D y texturas: incluidos en `models/` y `textures/` dentro del repositorio.


