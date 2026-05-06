#!/usr/bin/env bash
set -euo pipefail

# Compila y ejecuta ejercicios OpenGL del laboratorio desde cualquier carpeta de práctica.
# Uso (dentro de una carpeta como 01_OpenGLIntro):
#   ../build_and_run.sh
#   ../build_and_run.sh --build-only
#   ../build_and_run.sh --name mi_ejecutable

BUILD_ONLY=0
EXEC_NAME=""

while [[ $# -gt 0 ]]; do
  case "$1" in
    --build-only)
      BUILD_ONLY=1
      shift
      ;;
    --name)
      EXEC_NAME="${2:-}"
      if [[ -z "$EXEC_NAME" ]]; then
        echo "Error: --name requiere un valor"
        exit 1
      fi
      shift 2
      ;;
    -h|--help)
      cat <<'EOF'
Uso:
  ../build_and_run.sh [--build-only] [--name NOMBRE]

Opciones:
  --build-only   Solo compila, no ejecuta.
  --name NOMBRE  Nombre del ejecutable (por defecto: nombre de carpeta actual).
EOF
      exit 0
      ;;
    *)
      echo "Opción no reconocida: $1"
      exit 1
      ;;
  esac
done

CURRENT_DIR="$(pwd)"
PROJECT_DIR="$CURRENT_DIR/project1"
BIN_DIR="$PROJECT_DIR/bin"
OTHER_DIR="$PROJECT_DIR/other"

if [[ -z "$EXEC_NAME" ]]; then
  EXEC_NAME="$(basename "$CURRENT_DIR")"
fi

mkdir -p "$BIN_DIR" "$OTHER_DIR"

# Detecta raíz del repo (asume script ubicado en raíz).
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$SCRIPT_DIR"

if [[ ! -d "$ROOT_DIR/deps" ]]; then
  echo "Error: no se encontró carpeta deps en $ROOT_DIR"
  echo "Coloca build_and_run.sh en la raíz del repo stv-labopengl."
  exit 1
fi

if [[ ! -f "$ROOT_DIR/deps/glad/MSVC2022/src/glad.c" ]]; then
  echo "Error: no se encontró glad.c en deps/glad/MSVC2022/src/glad.c"
  exit 1
fi

if [[ -d "$ROOT_DIR/deps/glm/include" ]]; then
  GLM_INCLUDE_DIR="$ROOT_DIR/deps/glm/include"
elif [[ -d "$ROOT_DIR/deps/glm" ]]; then
  GLM_INCLUDE_DIR="$ROOT_DIR/deps/glm"
else
  echo "Error: no se encontró GLM en deps/glm/include ni deps/glm"
  exit 1
fi

if [[ -d "$ROOT_DIR/deps/glfw/MSVC2022/include" ]]; then
  GLFW_INCLUDE_DIR="$ROOT_DIR/deps/glfw/MSVC2022/include"
elif [[ -d "$ROOT_DIR/deps/glfw/MSVC2019/include" ]]; then
  GLFW_INCLUDE_DIR="$ROOT_DIR/deps/glfw/MSVC2019/include"
else
  echo "Error: no se encontró include de GLFW en deps/glfw/MSVC2022/include ni MSVC2019/include"
  exit 1
fi

if [[ -d "$ROOT_DIR/deps/glad/MSVC2022/include" ]]; then
  GLAD_INCLUDE_DIR="$ROOT_DIR/deps/glad/MSVC2022/include"
elif [[ -d "$ROOT_DIR/deps/glad/MSVC2019/include" ]]; then
  GLAD_INCLUDE_DIR="$ROOT_DIR/deps/glad/MSVC2019/include"
else
  echo "Error: no se encontró include de GLAD en deps/glad/MSVC2022/include ni MSVC2019/include"
  exit 1
fi

if [[ -d "$ROOT_DIR/deps/assimp/MSVC2022/include" ]]; then
  ASSIMP_INCLUDE_DIR="$ROOT_DIR/deps/assimp/MSVC2022/include"
elif [[ -d "$ROOT_DIR/deps/assimp/MSVC2019/include" ]]; then
  ASSIMP_INCLUDE_DIR="$ROOT_DIR/deps/assimp/MSVC2019/include"
else
  echo "Error: no se encontró include de Assimp en deps/assimp/MSVC2022/include ni MSVC2019/include"
  exit 1
fi

if [[ -d "$ROOT_DIR/deps/irrklang/include" ]]; then
  IRRKLANG_INCLUDE_DIR="$ROOT_DIR/deps/irrklang/include"
else
  IRRKLANG_INCLUDE_DIR=""
fi

IRRKLANG_STUB_DIR=""
IRRKLANG_INCLUDE_FLAG=()
IRRKLANG_CFLAGS=""
IRRKLANG_LIBS=""

# El principal siempre debe ser NN_Nombre.cpp 
mapfile -t MAIN_CPP_CANDIDATES < <(find "$CURRENT_DIR" -maxdepth 1 -type f -regextype posix-extended -regex ".*/[0-9]{2}_[^/]*\\.cpp" | sort)

if [[ ${#MAIN_CPP_CANDIDATES[@]} -eq 0 ]]; then
  echo "Error: no se encontró archivo principal con patrón NN_Nombre.cpp o NN-Nombre.cpp en $(basename "$CURRENT_DIR")"
  exit 1
fi

if [[ ${#MAIN_CPP_CANDIDATES[@]} -gt 1 ]]; then
  echo "Aviso: se encontraron varios archivos principales; se usará el primero:"
  printf '  - %s\n' "${MAIN_CPP_CANDIDATES[@]}"
fi

CPP_FILES=("${MAIN_CPP_CANDIDATES[0]}")

# Archivo complementario común para carga de texturas.
if [[ -f "$CURRENT_DIR/stb_image.cpp" ]]; then
  CPP_FILES+=("$CURRENT_DIR/stb_image.cpp")
fi

if ! command -v g++ >/dev/null 2>&1; then
  echo "Error: g++ no está instalado."
  echo "Instala con: sudo apt install g++"
  exit 1
fi

if ! command -v pkg-config >/dev/null 2>&1; then
  echo "Error: pkg-config no está instalado."
  echo "Instala con: sudo apt install pkg-config"
  exit 1
fi

if ! pkg-config --exists glfw3; then
  echo "Error: no se encontró glfw3 en el sistema (Linux)."
  echo "Instala con: sudo apt install libglfw3-dev"
  exit 1
fi

LOCAL_INCLUDE=()
if [[ -d "$CURRENT_DIR/include" ]]; then
  LOCAL_INCLUDE=("-I$CURRENT_DIR/include")
fi

if [[ -n "$IRRKLANG_INCLUDE_DIR" ]]; then
  if grep -q -e "irrKlang\.h" -e "irrklang\.h" "${CPP_FILES[@]}" 2>/dev/null; then
    if pkg-config --exists irrklang; then
      IRRKLANG_CFLAGS="$(pkg-config --cflags irrklang)"
      IRRKLANG_LIBS="$(pkg-config --libs irrklang)"
    elif command -v ldconfig >/dev/null 2>&1 && ldconfig -p 2>/dev/null | grep -Eq "lib(IrrKlang|irrKlang)\.so"; then
      IRRKLANG_LIBS="-lIrrKlang"
    else
      IRRKLANG_STUB_DIR="$OTHER_DIR/irrklang_stub"
      mkdir -p "$IRRKLANG_STUB_DIR"
      cat > "$IRRKLANG_STUB_DIR/irrKlang.h" <<'EOF'
#ifndef IRRKLANG_H
#define IRRKLANG_H

namespace irrklang {

enum E_SOUND_OUTPUT_DRIVER
{
    ESDL_AUTO_DETECT = 0
};

class ISoundEngine
{
public:
    void drop() {}
    void play2D(const char*, bool = false) {}
    void play3D(const char*, float = 0.0f, float = 0.0f, float = 0.0f, bool = false) {}
};

inline ISoundEngine* createIrrKlangDevice(E_SOUND_OUTPUT_DRIVER = ESDL_AUTO_DETECT, int = 0, const char* = nullptr, const char* = nullptr)
{
    static ISoundEngine engine;
    return &engine;
}

}

#endif
EOF
      IRRKLANG_INCLUDE_DIR="$IRRKLANG_STUB_DIR"
      IRRKLANG_INCLUDE_FLAG=("-I$IRRKLANG_INCLUDE_DIR")
      IRRKLANG_CFLAGS=""
      IRRKLANG_LIBS=""
      echo "Aviso: este proyecto usa irrKlang, pero no se encontró una librería Linux compatible."
      echo "Se compilará usando un stub temporal sin audio."
    fi
  fi
fi

if [[ ${#IRRKLANG_INCLUDE_FLAG[@]} -eq 0 && -n "$IRRKLANG_INCLUDE_DIR" ]]; then
  IRRKLANG_INCLUDE_FLAG=("-I$IRRKLANG_INCLUDE_DIR")
fi

# Incluye headers locales de dependencias del repo.
INCLUDE_FLAGS=(
  "${IRRKLANG_INCLUDE_FLAG[@]}"
  "-I$GLFW_INCLUDE_DIR"
  "-I$GLAD_INCLUDE_DIR"
  "-I$GLM_INCLUDE_DIR"
  "-I$ASSIMP_INCLUDE_DIR"
  "${LOCAL_INCLUDE[@]}"
)

# Nota: en Linux usamos glfw del sistema vía pkg-config.
GLFW_CFLAGS="$(pkg-config --cflags glfw3)"
GLFW_LIBS="$(pkg-config --libs glfw3)"

ASSIMP_CFLAGS=""
ASSIMP_LIBS=""
if pkg-config --exists assimp; then
  ASSIMP_CFLAGS="$(pkg-config --cflags assimp)"
  ASSIMP_LIBS="$(pkg-config --libs assimp)"
elif command -v ldconfig >/dev/null 2>&1 && ldconfig -p 2>/dev/null | grep -q "libassimp\\.so"; then
  ASSIMP_LIBS="-lassimp"
else
  echo "Error: no se encontró Assimp para enlazar en Linux."
  echo "Instala con: sudo apt install libassimp-dev"
  exit 1
fi

PLATFORM_DEFINES=()
if [[ "$(uname -s)" == "Linux" ]]; then
  PLATFORM_DEFINES+=("-Dsprintf_s=sprintf")
fi

OUT_EXE="$BIN_DIR/$EXEC_NAME"

echo "Compilando en: $OUT_EXE"
echo "Fuentes:"
printf '  - %s\n' "${CPP_FILES[@]}"

g++ -std=c++17 -O2 -g \
  "${CPP_FILES[@]}" \
  "$ROOT_DIR/deps/glad/MSVC2022/src/glad.c" \
  "${INCLUDE_FLAGS[@]}" \
  "${PLATFORM_DEFINES[@]}" \
  $GLFW_CFLAGS \
  $ASSIMP_CFLAGS \
  $IRRKLANG_CFLAGS \
  -o "$OUT_EXE" \
  $GLFW_LIBS $ASSIMP_LIBS $IRRKLANG_LIBS -ldl -lGL -lm -pthread

# Recursos compartidos esperados por varios ejercicios (rutas relativas como shaders/*)
for resource in shaders models textures; do
  if [[ -d "$ROOT_DIR/bin/$resource" && ! -e "$BIN_DIR/$resource" ]]; then
    ln -s "$ROOT_DIR/bin/$resource" "$BIN_DIR/$resource"
  fi
done

echo "Build OK."

if [[ "$BUILD_ONLY" -eq 1 ]]; then
  echo "Modo build-only: no se ejecuta."
  exit 0
fi

echo "Ejecutando..."
(
  cd "$BIN_DIR"
  "./$EXEC_NAME"
)
