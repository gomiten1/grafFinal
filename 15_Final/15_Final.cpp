/*
* 
* 15 - Proyecto final
*/

#include <iostream>
#include <stdlib.h>

// GLAD: Multi-Language GL/GLES/EGL/GLX/WGL Loader-Generator
// https://glad.dav1d.de/
#include <glad/glad.h>

// GLFW: https://www.glfw.org/
#include <GLFW/glfw3.h>

// GLM: OpenGL Math library
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Model loading classes
#include <shader_m.h>
#include <camera.h>
#include <model.h>
#include <animatedmodel.h>
#include <material.h>
#include <light.h>
#include <cubemap.h>

#include <irrKlang.h>
using namespace irrklang;

// Functions
bool Start();
bool Update();

// Definición de callbacks
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);

// Gobals
GLFWwindow* window;

// Tamaño en pixeles de la ventana
const unsigned int SCR_WIDTH = 1024;
const unsigned int SCR_HEIGHT = 768;

// Definición de cámara (posición en XYZ)
Camera camera(glm::vec3(0.0f, 10.0f, 60.0f));
Camera camera3rd(glm::vec3(0.0f, 0.0f, 0.0f));

// Controladores para el movimiento del mouse
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// Variables para la velocidad de reproducción
// de la animación
float deltaTime = 0.0f;
float lastFrame = 0.0f;
float elapsedTime = 0.0f;

glm::vec3 position(0.0f,0.0f, 0.0f);
glm::vec3 forwardView(0.0f, 0.0f, 1.0f);
	// Distancia de la cámara a la posición del personaje
	float     trdpersonOffset = 2.5f;
	// Subir altura de la cámara en 4 unidades
	float     trdpersonHeightOffset = 6.0f;
	// Altura del personaje 
	float     trdpersonCharacterYOffset = 4.3f;
	float     scaleV = 0.025f; // legacy scalar kept for compatibility
	float     rotateCharacter = 0.0f;

// Shaders
Shader *mLightsShader;        // Phong con múltiples luces
Shader *basicPhongShader;      // Phong simple (1 luz)
Shader *fresnelShader;         // Fresnel
Shader *proceduralShader;
Shader *wavesShader;

Shader *cubemapShader;
Shader *dynamicShader;

// Tipo de iluminación (0 = Phong MultiLights, 1 = Fresnel, 2 = Phong Simple, 3 = Phong + Fresnel)
int lightingMode = 0;

// Carga la información del modelo
Model* barconew;
Model* icebergChico;
Model* icebergGrande;
Model* iglu;
Model* zorro;
Model* ola;

glm::vec3 barconewPosition(6.0f, 0.0f, 0.0f);
glm::vec3 icebergChicoPosition(60.0f, -2.0f, -20.0f);
glm::vec3 icebergGrandePosition(-45.0f, -1.0f, -40.0f);
glm::vec3 igluPosition(-4.8f, 0.0f, -3.5f);
glm::vec3 zorroPosition(1.8f, 0.0f, -5.6f);
glm::vec3 olaPosition(0.0f, 0.0f, 0.0f);

glm::vec3 barconewRotation(0.0f, 0.0f, 0.0f);
glm::vec3 icebergChicoRotation(0.0f, 0.0f, 0.0f);
glm::vec3 icebergGrandeRotation(0.0f, 0.0f, 0.0f);
glm::vec3 igluRotation(0.0f, 0.0f, 0.0f);
glm::vec3 zorroRotation(0.0f, 0.0f, 0.0f);
glm::vec3 olaRotation(-90.0f, 0.0f, 0.0f);

glm::vec3 barconewScale(1.0f, 1.0f, 1.0f);
glm::vec3 icebergChicoScale(25.0f, 25.0f, 25.0f);
glm::vec3 icebergGrandeScale(12.0f, 12.0f, 12.0f);
glm::vec3 igluScale(1.0f, 1.0f, 1.0f);
glm::vec3 zorroScale(1.0f, 1.0f, 1.0f);
glm::vec3 olaScale(20.0f, 20.0f, 20.0f);

glm::mat4 barconewModel = glm::mat4(1.0f);
glm::mat4 icebergChicoModel = glm::mat4(1.0f);
glm::mat4 icebergGrandeModel = glm::mat4(1.0f);
glm::mat4 igluModel = glm::mat4(1.0f);
glm::mat4 zorroModel = glm::mat4(1.0f);
glm::mat4 olaModel = glm::mat4(1.0f);

Model* leonMarino;
Model* osoA;
Model* pexDorado;
Model* bear;
Model* cargo;
Model* fish;
Model* gas;
Model* icebergA;
Model* icebergD;
Model* oilPump;
Model* orca;
AnimatedModel* pez;
Model* pinguino;
Model* reno;
Model* rig;
Model* seal;
Model* ship;
Model* tanqueDerramado;
Model* tanqueGrande;
Model* tanques;
Model* titanic;
Model* wolf;
Model* trabajadorAnimado;
Model* trabajadoraAnimada;
Model* personaje;

glm::vec3 leonMarinoPosition(-18.0f, 0.0f, 12.0f);
glm::vec3 osoAPosition(-12.0f, 0.0f, 12.0f);
glm::vec3 pexDoradoPosition(-6.0f, 0.0f, 12.0f);
glm::vec3 bearPosition(0.0f, 0.0f, 12.0f);
glm::vec3 cargoPosition(6.0f, 0.0f, 12.0f);
glm::vec3 fishPosition(12.0f, 0.0f, 12.0f);
glm::vec3 gasPosition(-18.0f, 0.0f, 6.0f);
glm::vec3 icebergAPosition(-25.0f, -1.0f, 25.0f);
glm::vec3 icebergDPosition(15.0f, 0.0f, 15.0f);
glm::vec3 oilPumpPosition(0.0f, 0.0f, 6.0f);
glm::vec3 orcaPosition(6.0f, 0.0f, 6.0f);
glm::vec3 pezPosition(12.0f, 0.0f, 6.0f);
glm::vec3 pinguinoPosition(-18.0f, 0.0f, 0.0f);
glm::vec3 renoPosition(-12.0f, 0.0f, 0.0f);
glm::vec3 rigPosition(-6.0f, 0.0f, 0.0f);
glm::vec3 sealPosition(0.0f, 0.0f, 0.0f);
glm::vec3 shipPosition(6.0f, 0.0f, 0.0f);
glm::vec3 tanqueDerramadoPosition(12.0f, 0.0f, 0.0f);
glm::vec3 tanqueGrandePosition(-18.0f, 0.0f, -6.0f);
glm::vec3 tanquesPosition(-12.0f, 0.0f, -6.0f);
glm::vec3 titanicPosition(-6.0f, 0.0f, -6.0f);
glm::vec3 wolfPosition(0.0f, 0.0f, -6.0f);
glm::vec3 trabajadorAnimadoPosition(6.0f, 0.0f, -6.0f);
glm::vec3 trabajadoraAnimadaPosition(12.0f, 0.0f, -6.0f);

glm::vec3 leonMarinoRotation(0.0f, 0.0f, 0.0f);
glm::vec3 osoARotation(0.0f, 0.0f, 0.0f);
glm::vec3 pexDoradoRotation(0.0f, 0.0f, 0.0f);
glm::vec3 bearRotation(0.0f, 0.0f, 0.0f);
glm::vec3 cargoRotation(0.0f, 0.0f, 0.0f);
glm::vec3 fishRotation(0.0f, 0.0f, 0.0f);
glm::vec3 gasRotation(0.0f, 0.0f, 0.0f);
glm::vec3 icebergARotation(-90.0f, 0.0f, 0.0f);
glm::vec3 icebergDRotation(-90.0f, 0.0f, 0.0f);
glm::vec3 oilPumpRotation(0.0f, 0.0f, 0.0f);
glm::vec3 orcaRotation(0.0f, 0.0f, 0.0f);
glm::vec3 pezRotation(0.0f, 0.0f, 0.0f);
glm::vec3 pinguinoRotation(0.0f, 0.0f, 0.0f);
glm::vec3 renoRotation(0.0f, 0.0f, 0.0f);
glm::vec3 rigRotation(0.0f, 0.0f, 0.0f);
glm::vec3 sealRotation(0.0f, 0.0f, 0.0f);
glm::vec3 shipRotation(0.0f, 0.0f, 0.0f);
glm::vec3 tanqueDerramadoRotation(0.0f, 0.0f, 0.0f);
glm::vec3 tanqueGrandeRotation(0.0f, 0.0f, 0.0f);
glm::vec3 tanquesRotation(0.0f, 0.0f, 0.0f);
glm::vec3 titanicRotation(0.0f, 0.0f, 0.0f);
glm::vec3 wolfRotation(0.0f, 0.0f, 0.0f);
glm::vec3 trabajadorAnimadoRotation(0.0f, 0.0f, 0.0f);
glm::vec3 trabajadoraAnimadaRotation(0.0f, 0.0f, 0.0f);

glm::vec3 leonMarinoScale(1.0f, 1.0f, 1.0f);
glm::vec3 osoAScale(1.0f, 1.0f, 1.0f);
glm::vec3 pexDoradoScale(1.0f, 1.0f, 1.0f);
glm::vec3 bearScale(1.0f, 1.0f, 1.0f);
glm::vec3 cargoScale(1.0f, 1.0f, 1.0f);
glm::vec3 fishScale(1.0f, 1.0f, 1.0f);
glm::vec3 gasScale(1.0f, 1.0f, 1.0f);
glm::vec3 icebergAScale(5.0f, 5.0f, 5.0f);
glm::vec3 icebergDScale(1.0f, 1.0f, 1.0f);
glm::vec3 oilPumpScale(1.0f, 1.0f, 1.0f);
glm::vec3 orcaScale(1.0f, 1.0f, 1.0f);
glm::vec3 pezScale(1.0f, 1.0f, 1.0f);
glm::vec3 pinguinoScale(1.0f, 1.0f, 1.0f);
glm::vec3 renoScale(1.0f, 1.0f, 1.0f);
glm::vec3 rigScale(1.0f, 1.0f, 1.0f);
glm::vec3 sealScale(1.0f, 1.0f, 1.0f);
glm::vec3 shipScale(1.0f, 1.0f, 1.0f);
glm::vec3 tanqueDerramadoScale(1.0f, 1.0f, 1.0f);
glm::vec3 tanqueGrandeScale(1.0f, 1.0f, 1.0f);
glm::vec3 tanquesScale(1.0f, 1.0f, 1.0f);
glm::vec3 titanicScale(1.0f, 1.0f, 1.0f);
glm::vec3 wolfScale(1.0f, 1.0f, 1.0f);
glm::vec3 trabajadorAnimadoScale(1.0f, 1.0f, 1.0f);
glm::vec3 trabajadoraAnimadaScale(1.0f, 1.0f, 1.0f);

glm::mat4 leonMarinoModel = glm::mat4(1.0f);
glm::mat4 osoAModel = glm::mat4(1.0f);
glm::mat4 pexDoradoModel = glm::mat4(1.0f);
glm::mat4 bearModel = glm::mat4(1.0f);
glm::mat4 cargoModel = glm::mat4(1.0f);
glm::mat4 fishModel = glm::mat4(1.0f);
glm::mat4 gasModel = glm::mat4(1.0f);
glm::mat4 icebergAModel = glm::mat4(1.0f);
glm::mat4 icebergDModel = glm::mat4(1.0f);
glm::mat4 oilPumpModel = glm::mat4(1.0f);
glm::mat4 orcaModel = glm::mat4(1.0f);
glm::mat4 pezModel = glm::mat4(1.0f);
glm::mat4 pinguinoModel = glm::mat4(1.0f);
glm::mat4 renoModel = glm::mat4(1.0f);
glm::mat4 renoPlayerModel = glm::mat4(1.0f);
glm::mat4 rigModel = glm::mat4(1.0f);
glm::mat4 sealModel = glm::mat4(1.0f);
glm::mat4 shipModel = glm::mat4(1.0f);
glm::mat4 tanqueDerramadoModel = glm::mat4(1.0f);
glm::mat4 tanqueGrandeModel = glm::mat4(1.0f);
glm::mat4 tanquesModel = glm::mat4(1.0f);
glm::mat4 titanicModel = glm::mat4(1.0f);
glm::mat4 wolfModel = glm::mat4(1.0f);
glm::mat4 trabajadorAnimadoModel = glm::mat4(1.0f);
glm::mat4 trabajadoraAnimadaModel = glm::mat4(1.0f);
glm::mat4 personajePlayerModel = glm::mat4(1.0f);

int selectedModelIndex = 0;

glm::mat4 BuildModelMatrix(const glm::vec3& positionValue, const glm::vec3& rotationValue, const glm::vec3& scaleValue) {
	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, positionValue);
	model = glm::rotate(model, glm::radians(rotationValue.x), glm::vec3(1.0f, 0.0f, 0.0f));
	model = glm::rotate(model, glm::radians(rotationValue.y), glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::rotate(model, glm::radians(rotationValue.z), glm::vec3(0.0f, 0.0f, 1.0f));
	model = glm::scale(model, scaleValue);
	return model;
}

void TranslateSelectedModel(const glm::vec3& delta) {
	switch (selectedModelIndex) {
	case 0: barconewPosition += delta; break;
	case 1: icebergChicoPosition += delta; break;
	case 2: icebergGrandePosition += delta; break;
	case 3: igluPosition += delta; break;
	case 4: zorroPosition += delta; break;
	default: break;
	}
}

void RotateSelectedModel(const glm::vec3& delta) {
	switch (selectedModelIndex) {
	case 0: barconewRotation += delta; break;
	case 1: icebergChicoRotation += delta; break;
	case 2: icebergGrandeRotation += delta; break;
	case 3: igluRotation += delta; break;
	case 4: zorroRotation += delta; break;
	default: break;
	}
}

void ScaleSelectedModel(float delta) {
	const float minScale = 0.1f;
	switch (selectedModelIndex) {
	case 0: barconewScale = glm::max(barconewScale + glm::vec3(delta), glm::vec3(minScale)); break;
	case 1: icebergChicoScale = glm::max(icebergChicoScale + glm::vec3(delta), glm::vec3(minScale)); break;
	case 2: icebergGrandeScale = glm::max(icebergGrandeScale + glm::vec3(delta), glm::vec3(minScale)); break;
	case 3: igluScale = glm::max(igluScale + glm::vec3(delta), glm::vec3(minScale)); break;
	case 4: zorroScale = glm::max(zorroScale + glm::vec3(delta), glm::vec3(minScale)); break;
	default: break;
	}
}

void SetSelectedModelIndex(int index) {
	selectedModelIndex = index;
	const char* modelNames[] = { "barconew", "icebergChico", "icebergGrande", "iglu", "zorro" };
	std::cout << "Selected model: " << modelNames[selectedModelIndex] << std::endl;
}

void UpdateModelMatrices() {
	barconewModel = BuildModelMatrix(barconewPosition, barconewRotation, barconewScale);
	icebergChicoModel = BuildModelMatrix(icebergChicoPosition, icebergChicoRotation, icebergChicoScale);
	icebergGrandeModel = BuildModelMatrix(icebergGrandePosition, icebergGrandeRotation, icebergGrandeScale);
	igluModel = BuildModelMatrix(igluPosition, igluRotation, igluScale);
	zorroModel = BuildModelMatrix(zorroPosition, zorroRotation, zorroScale);
	olaModel = BuildModelMatrix(olaPosition, olaRotation, olaScale);
	leonMarinoModel = BuildModelMatrix(leonMarinoPosition, leonMarinoRotation, leonMarinoScale);
	osoAModel = BuildModelMatrix(osoAPosition, osoARotation, osoAScale);
	pexDoradoModel = BuildModelMatrix(pexDoradoPosition, pexDoradoRotation, pexDoradoScale);
	bearModel = BuildModelMatrix(bearPosition, bearRotation, bearScale);
	cargoModel = BuildModelMatrix(cargoPosition, cargoRotation, cargoScale);
	fishModel = BuildModelMatrix(fishPosition, fishRotation, fishScale);
	gasModel = BuildModelMatrix(gasPosition, gasRotation, gasScale);
	icebergAModel = BuildModelMatrix(icebergAPosition, icebergARotation, icebergAScale);
	icebergDModel = BuildModelMatrix(icebergDPosition, icebergDRotation, icebergDScale);
	oilPumpModel = BuildModelMatrix(oilPumpPosition, oilPumpRotation, oilPumpScale);
	orcaModel = BuildModelMatrix(orcaPosition, orcaRotation, orcaScale);
	pezModel = BuildModelMatrix(pezPosition, pezRotation, pezScale);
	pinguinoModel = BuildModelMatrix(pinguinoPosition, pinguinoRotation, pinguinoScale);
	renoModel = BuildModelMatrix(renoPosition, renoRotation, renoScale);

	// Matriz para el reno-controlable (jugador)
	renoPlayerModel = BuildModelMatrix(position, glm::vec3(0.0f, rotateCharacter, 0.0f), renoScale);
	rigModel = BuildModelMatrix(rigPosition, rigRotation, rigScale);
	sealModel = BuildModelMatrix(sealPosition, sealRotation, sealScale);
	shipModel = BuildModelMatrix(shipPosition, shipRotation, shipScale);
	tanqueDerramadoModel = BuildModelMatrix(tanqueDerramadoPosition, tanqueDerramadoRotation, tanqueDerramadoScale);
	tanqueGrandeModel = BuildModelMatrix(tanqueGrandePosition, tanqueGrandeRotation, tanqueGrandeScale);
	tanquesModel = BuildModelMatrix(tanquesPosition, tanquesRotation, tanquesScale);
	titanicModel = BuildModelMatrix(titanicPosition, titanicRotation, titanicScale);
	wolfModel = BuildModelMatrix(wolfPosition, wolfRotation, wolfScale);
	trabajadorAnimadoModel = BuildModelMatrix(trabajadorAnimadoPosition, trabajadorAnimadoRotation, trabajadorAnimadoScale);
	trabajadoraAnimadaModel = BuildModelMatrix(trabajadoraAnimadaPosition, trabajadoraAnimadaRotation, trabajadoraAnimadaScale);
	personajePlayerModel = BuildModelMatrix(position + glm::vec3(0.0f, trdpersonCharacterYOffset, 0.0f), glm::vec3(0.0f, rotateCharacter, 0.0f), glm::vec3(0.01f, 0.01f, 0.01f));
}

float tradius = 10.0f;
float theta = 0.0f;
float alpha = 0.0f;

// Cubemap
CubeMap *mainCubeMap;

// Light para modo simple
Light gSimpleLight;

// Luces para modo MultiLights
std::vector<Light> gLights;

// Materiales
Material material01;

float proceduralTime = 0.0f;
float wavesTime = 0.0f;

// Audio
ISoundEngine *SoundEngine = createIrrKlangDevice();

// selección de cámara
bool    activeCamera = 1; // activamos la primera cámara

// Entrada a función principal
int main()
{
	if (!Start())
		return -1;

	/* Loop until the user closes the window */
	while (!glfwWindowShouldClose(window))
	{
		if (!Update())
			break;
	}

	glfwTerminate();
	return 0;

}

bool Start() {
	// Inicialización de GLFW

	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Creación de la ventana con GLFW
	window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Animation", NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return false;
	}
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);

	// Ocultar el cursor mientras se rota la escena
	// glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// glad: Cargar todos los apuntadores
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return false;
	}

	// Activación de buffer de profundidad
	glEnable(GL_DEPTH_TEST);

	// Compilación y enlace de shaders
	mLightsShader = new Shader("shaders/11_PhongShaderMultLights.vs", "shaders/11_PhongShaderMultLights.fs");
	basicPhongShader = new Shader("shaders/11_BasicPhongShader.vs", "shaders/11_BasicPhongShader.fs");
	fresnelShader = new Shader("shaders/11_Fresnel.vs", "shaders/11_Fresnel.fs");
	proceduralShader = new Shader("shaders/12_ProceduralAnimation.vs", "shaders/12_ProceduralAnimation.fs");
	wavesShader = new Shader("shaders/13_wavesAnimation.vs", "shaders/13_wavesAnimation.fs");
	cubemapShader = new Shader("shaders/10_vertex_cubemap.vs", "shaders/10_fragment_cubemap.fs");
	dynamicShader = new Shader("shaders/10_vertex_skinning-IT.vs", "shaders/10_fragment_skinning-IT.fs");

	// Máximo número de huesos: 100
	dynamicShader->setBonesIDs(MAX_RIGGING_BONES);

	// Dibujar en malla de alambre
	// glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);

	int loadedModels = 0;

	// Cargar modelos individuales del directorio bin/models
	std::cout << "Attempting to load: models/barcopes.fbx" << std::endl;
	barconew = new Model("models/barcopes.fbx");
	loadedModels++;

	std::cout << "Attempting to load: models/icebergChico.fbx" << std::endl;
	icebergChico = new Model("models/icebergChico.fbx");
	loadedModels++;

	std::cout << "Attempting to load: models/icebergGrande.fbx" << std::endl;
	icebergGrande = new Model("models/icebergGrande.fbx");
	loadedModels++;

	std::cout << "Attempting to load: models/iglu.fbx" << std::endl;
	iglu = new Model("models/iglu.fbx");
	loadedModels++;

	std::cout << "Attempting to load: models/zorro.fbx" << std::endl;
	zorro = new Model("models/zorro.fbx");
	loadedModels++;

	std::cout << "Attempting to load: models/ola.fbx" << std::endl;
	ola = new Model("models/ola.fbx");
	loadedModels++;

	std::cout << "Attempting to load: models/LEONMARINO.fbx" << std::endl;
	leonMarino = new Model("models/LEONMARINO.fbx");
	loadedModels++;

	std::cout << "Attempting to load: models/OsoA.fbx" << std::endl;
	osoA = new Model("models/OsoA.fbx");
	loadedModels++;

	std::cout << "Attempting to load: models/PEXDORADO.fbx" << std::endl;
	pexDorado = new Model("models/PEXDORADO.fbx");
	loadedModels++;

	std::cout << "Attempting to load: models/bear.fbx" << std::endl;
	bear = new Model("models/bear.fbx");
	loadedModels++;

	std::cout << "Attempting to load: models/cargo.fbx" << std::endl;
	cargo = new Model("models/cargo.fbx");
	loadedModels++;

	std::cout << "Attempting to load: models/fish.fbx" << std::endl;
	fish = new Model("models/fish.fbx");
	loadedModels++;

	std::cout << "Attempting to load: models/gas.fbx" << std::endl;
	gas = new Model("models/gas.fbx");
	loadedModels++;

	std::cout << "Attempting to load: models/icebergA.fbx" << std::endl;
	icebergA = new Model("models/icebergA.fbx");
	loadedModels++;

	std::cout << "Attempting to load: models/icebergD.fbx" << std::endl;
	icebergD = new Model("models/icebergD.fbx");
	loadedModels++;

	std::cout << "Attempting to load: models/oilPump.fbx" << std::endl;
	oilPump = new Model("models/oilPump.fbx");
	loadedModels++;

	std::cout << "Attempting to load: models/orca.fbx" << std::endl;
	orca = new Model("models/orca.fbx");
	loadedModels++;

	std::cout << "Attempting to load: models/pez.fbx" << std::endl;
	pez = new AnimatedModel("models/pez.fbx");
	loadedModels++;

	std::cout << "Attempting to load: models/pinguino.fbx" << std::endl;
	pinguino = new Model("models/pinguino.fbx");
	loadedModels++;

	std::cout << "Attempting to load: models/reno.fbx" << std::endl;
	reno = new Model("models/reno.fbx");
	loadedModels++;

	

	

	std::cout << "Attempting to load: models/ship.fbx" << std::endl;
	ship = new Model("models/ship.fbx");
	loadedModels++;

	std::cout << "Attempting to load: models/tanqueDerramado.fbx" << std::endl;
	tanqueDerramado = new Model("models/tanqueDerramado.fbx");
	loadedModels++;

	std::cout << "Attempting to load: models/tanqueGrande.fbx" << std::endl;
	tanqueGrande = new Model("models/tanqueGrande.fbx");
	loadedModels++;

	std::cout << "Attempting to load: models/tanques.fbx" << std::endl;
	tanques = new Model("models/tanques.fbx");
	loadedModels++;

	std::cout << "Attempting to load: models/titanic.fbx" << std::endl;
	titanic = new Model("models/titanic.fbx");
	loadedModels++;

	std::cout << "Attempting to load: models/wolf.fbx" << std::endl;
	wolf = new Model("models/wolf.fbx");
	loadedModels++;

	std::cout << "Attempting to load: models/trabajadores/personaje.fbx" << std::endl;
	personaje = new Model("models/trabajadores/personaje.fbx");
	loadedModels++;

	std::cout << "Attempting to load: models/trabajadores/Trabajador_animado.fbx" << std::endl;
	trabajadorAnimado = new Model("models/trabajadores/Trabajador.fbx");
	loadedModels++;

	std::cout << "Attempting to load: models/trabajadores/Trabajadora_animada.fbx" << std::endl;
	trabajadoraAnimada = new Model("models/trabajadores/Trabajadora.fbx");
	loadedModels++;

	std::cout << "Attempting to load: models/rig.fbx" << std::endl;
	rig = new Model("models/rig.fbx");
	loadedModels++;

	std::cout << "Attempting to load: models/seal.fbx" << std::endl;
	seal = new Model("models/seal.fbx");
	loadedModels++;

	std::cout << "Loaded " << loadedModels << " individual models" << std::endl;

	// Cubemap
	vector<std::string> faces
	{
		"textures/cubemap/03/posx.jpg",
		"textures/cubemap/03/negx.jpg",
		"textures/cubemap/03/posy.jpg",
		"textures/cubemap/03/negy.jpg",
		"textures/cubemap/03/posz.jpg",
		"textures/cubemap/03/negz.jpg"
	};
	mainCubeMap = new CubeMap();
	mainCubeMap->loadCubemap(faces);

	camera3rd.Position = position;
	camera3rd.Position.y += trdpersonHeightOffset;
	camera3rd.Position -= trdpersonOffset * forwardView;
	camera3rd.Front = forwardView;

	// Lights configuration
	
	Light light01;
	light01.Position = glm::vec3(5.0f, 2.0f, 5.0f);
	light01.Color = glm::vec4(0.2f, 0.0f, 0.0f, 1.0f);
	gLights.push_back(light01);

	Light light02;
	light02.Position = glm::vec3(-5.0f, 2.0f, 5.0f);
	light02.Color = glm::vec4(0.0f, 0.2f, 0.0f, 1.0f);
	gLights.push_back(light02);

	Light light03;
	light03.Position = glm::vec3(5.0f, 2.0f, -5.0f);
	light03.Color = glm::vec4(0.0f, 0.0f, 0.2f, 1.0f);
	gLights.push_back(light03);

	Light light04;
	light04.Position = glm::vec3(-5.0f, 2.0f, -5.0f);
	light04.Color = glm::vec4(0.2f, 0.2f, 0.0f, 1.0f);
	gLights.push_back(light04);
	
	// Configure simple light for BasicPhongShader mode
	gSimpleLight.Position = glm::vec3(5.0f, 5.0f, 5.0f);
	gSimpleLight.Color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
	gSimpleLight.Power = glm::vec4(60.0f, 60.0f, 60.0f, 1.0f);
	gSimpleLight.alphaIndex = 10;
	gSimpleLight.distance = 5.0f;
	
	// SoundEngine->play2D("sound/EternalGarden.mp3", true);

	return true;
}


void SetLightUniformInt(Shader* shader, const char* propertyName, size_t lightIndex, int value) {
	std::ostringstream ss;
	ss << "allLights[" << lightIndex << "]." << propertyName;
	std::string uniformName = ss.str();

	shader->setInt(uniformName.c_str(), value);
}
void SetLightUniformFloat(Shader* shader, const char* propertyName, size_t lightIndex, float value) {
	std::ostringstream ss;
	ss << "allLights[" << lightIndex << "]." << propertyName;
	std::string uniformName = ss.str();

	shader->setFloat(uniformName.c_str(), value);
}
void SetLightUniformVec4(Shader* shader, const char* propertyName, size_t lightIndex, glm::vec4 value) {
	std::ostringstream ss;
	ss << "allLights[" << lightIndex << "]." << propertyName;
	std::string uniformName = ss.str();

	shader->setVec4(uniformName.c_str(), value);
}
void SetLightUniformVec3(Shader* shader, const char* propertyName, size_t lightIndex, glm::vec3 value) {
	std::ostringstream ss;
	ss << "allLights[" << lightIndex << "]." << propertyName;
	std::string uniformName = ss.str();

	shader->setVec3(uniformName.c_str(), value);
}


bool Update() {
	// Cálculo del framerate
	float currentFrame = (float)glfwGetTime();
	deltaTime = currentFrame - lastFrame;
	lastFrame = currentFrame;

	// Procesa la entrada del teclado o mouse
	processInput(window);

	// Renderizado R - G - B - A
	glClearColor(1.0f, 1.0f, 1.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	UpdateModelMatrices();

	glm::mat4 projection;
	glm::mat4 view;
	glm::vec3 eyePosition;

	if (activeCamera) {
		// Cámara en primera persona
		projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 10000.0f);
		view = camera.GetViewMatrix();
		eyePosition = camera.Position;
	}
	else {
		// cámara en tercera persona
		projection = glm::perspective(glm::radians(camera3rd.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 10000.0f);
		view = camera3rd.GetViewMatrix();
		eyePosition = camera3rd.Position;
	}

	// Cubemap (fondo)
	{
		mainCubeMap->drawCubeMap(*cubemapShader, projection, view);
	}
	
	// Renderizar modelos en círculo
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		
		// Seleccionar shader según modo de iluminación
		Shader* activeShader;
		if (lightingMode == 1) {
			activeShader = fresnelShader;
		} else if (lightingMode == 2) {
			activeShader = basicPhongShader;
		} else {
			activeShader = mLightsShader;
		}
		activeShader->use();

		activeShader->setMat4("projection", projection);
		activeShader->setMat4("view", view);

		if (lightingMode == 0 || lightingMode == 3) {
			// Configuración para Phong MultiLights
			// Configuramos propiedades de fuentes de luz
			mLightsShader->setInt("numLights", (int)gLights.size());
			for (size_t i = 0; i < gLights.size(); ++i) {
				SetLightUniformVec3(mLightsShader, "Position", i, gLights[i].Position);
				SetLightUniformVec3(mLightsShader, "Direction", i, gLights[i].Direction);
				SetLightUniformVec4(mLightsShader, "Color", i, gLights[i].Color);
				SetLightUniformVec4(mLightsShader, "Power", i, gLights[i].Power);
				SetLightUniformInt(mLightsShader, "alphaIndex", i, gLights[i].alphaIndex);
				SetLightUniformFloat(mLightsShader, "distance", i, gLights[i].distance);
			}
			
			mLightsShader->setVec3("eye", eyePosition);

			// Aplicamos propiedades materiales
			mLightsShader->setVec4("MaterialAmbientColor", material01.ambient);
			mLightsShader->setVec4("MaterialDiffuseColor", material01.diffuse);
			mLightsShader->setVec4("MaterialSpecularColor", material01.specular);
			mLightsShader->setFloat("transparency", material01.transparency);
		}
		else if (lightingMode == 1) {
			// Configuración para Fresnel - bindear cubemap una sola vez
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_CUBE_MAP, mainCubeMap->textureID);
			fresnelShader->setInt("cubetex", 0);  // Texture unit 0
			fresnelShader->setVec3("tintColor", glm::vec3(1.0f, 1.0f, 1.0f));
			fresnelShader->setFloat("tintStrength", 0.0f);

			// Configurar uniforms Fresnel una vez por frame
			fresnelShader->setVec3("cameraPosition", eyePosition);
			fresnelShader->setFloat("mRefractionRatio", 1.0f / 1.333f);  // Agua / vidrio ligero
			fresnelShader->setFloat("_Bias", 0.1f);
			fresnelShader->setFloat("_Scale", 0.15f);
			fresnelShader->setFloat("_Power", 1.5f);
		}
		else {
			// Configuración para Phong Simple (1 luz)
			basicPhongShader->setVec4("LightColor", gSimpleLight.Color);
			basicPhongShader->setVec4("LightPower", gSimpleLight.Power);
			basicPhongShader->setInt("alphaIndex", gSimpleLight.alphaIndex);
			basicPhongShader->setFloat("distance", gSimpleLight.distance);
			basicPhongShader->setVec3("lightPosition", gSimpleLight.Position);
			basicPhongShader->setVec3("lightDirection", gSimpleLight.Direction);
			basicPhongShader->setVec3("eye", eyePosition);

			// Aplicamos propiedades materiales
			basicPhongShader->setVec4("MaterialAmbientColor", material01.ambient);
			basicPhongShader->setVec4("MaterialDiffuseColor", material01.diffuse);
			basicPhongShader->setVec4("MaterialSpecularColor", material01.specular);
			basicPhongShader->setFloat("transparency", material01.transparency);
		}
		/*

		activeShader->setMat4("model", barconewModel);
		barconew->Draw(*activeShader);
		*/
		activeShader->setMat4("model", icebergChicoModel);
		icebergChico->Draw(*activeShader);

		activeShader->setMat4("model", icebergGrandeModel);
		icebergGrande->Draw(*activeShader);

		// Draw the wave mesh using the waves shader (procedural animation)
		wavesShader->use();

		// Activamos para objetos transparentes
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		wavesShader->setMat4("projection", projection);
		wavesShader->setMat4("view", view);
		wavesShader->setMat4("model", olaModel);

		// parámatros de la ola
		wavesShader->setFloat("time", wavesTime);
		wavesShader->setFloat("radius", 2.0f);
		wavesShader->setFloat("height", 5.0f);

		ola->Draw(*wavesShader);
		wavesTime += 0.01f;

		// restore previously active shader
		activeShader->use();

		/*pez->UpdateAnimation(deltaTime);
		dynamicShader->use();
		dynamicShader->setMat4("projection", projection);
		dynamicShader->setMat4("view", view);
		dynamicShader->setMat4("model", pezModel);
		dynamicShader->setMat4("gBones", MAX_RIGGING_BONES, pez->gBones);
		pez->Draw(*dynamicShader);
		// restore active shader
		activeShader->use();*/

		/*
		activeShader->setMat4("model", igluModel);
		iglu->Draw(*activeShader);

		activeShader->setMat4("model", zorroModel);
		zorro->Draw(*activeShader);

		
		activeShader->setMat4("model", leonMarinoModel);
		leonMarino->Draw(*activeShader);

		activeShader->setMat4("model", osoAModel);
		osoA->Draw(*activeShader);

		activeShader->setMat4("model", pexDoradoModel);
		pexDorado->Draw(*activeShader);

		activeShader->setMat4("model", bearModel);
		bear->Draw(*activeShader);

		activeShader->setMat4("model", cargoModel);
		cargo->Draw(*activeShader);

		activeShader->setMat4("model", fishModel);
		fish->Draw(*activeShader);

		activeShader->setMat4("model", gasModel);
		gas->Draw(*activeShader);*/

		// =========================================================
		// CLÚSTER COMPACTO (Suelo de tipo D + Montaña tipo A)
		// =========================================================

		// Reducimos las distancias entre X y Z para que estén pegaditos
		// Escala sugerida: 2.5f para que sea un clúster pequeño

		// Placa 1: Frontal izquierda
		glm::mat4 d1 = BuildModelMatrix(glm::vec3(-2.0f, 0.0f, 15.0f), icebergDRotation, glm::vec3(2.5f));
		activeShader->setMat4("model", d1);
		icebergD->Draw(*activeShader);

		// Placa 2: Frontal derecha (solo 4 unidades de diferencia en X)
		glm::mat4 d2 = BuildModelMatrix(glm::vec3(2.0f, -0.1f, 15.5f), icebergDRotation + glm::vec3(0, 15, 0), glm::vec3(2.2f));
		activeShader->setMat4("model", d2);
		icebergD->Draw(*activeShader);

		// Placa 3: Trasera izquierda
		glm::mat4 d3 = BuildModelMatrix(glm::vec3(-2.5f, -0.05f, 19.0f), icebergDRotation + glm::vec3(0, -10, 0), glm::vec3(2.8f));
		activeShader->setMat4("model", d3);
		icebergD->Draw(*activeShader);

		// Placa 4: Trasera derecha
		glm::mat4 d4 = BuildModelMatrix(glm::vec3(2.5f, 0.0f, 19.5f), icebergDRotation + glm::vec3(0, 45, 0), glm::vec3(2.3f));
		activeShader->setMat4("model", d4);
		icebergD->Draw(*activeShader);

		// --- MONTAÑA CENTRAL COMPACTA (TIPO A) ---
		// Ahora con escala 3.0 para que destaque pero no sea gigante
		glm::mat4 aCentro = BuildModelMatrix(glm::vec3(0.0f, 0.2f, 17.5f), icebergARotation, glm::vec3(3.5f));
		activeShader->setMat4("model", aCentro);
		icebergA->Draw(*activeShader);

		// =========================================================
		// PISO DE MOSAICOS PEQUEÑOS (Tipo D - Lado del Clúster)
		// =========================================================

		// Escala pequeña para que parezcan fragmentos de hielo
		glm::vec3 miniScale(1.5f);

		// Fila 1 (Alineada con el frente del clúster)
		activeShader->setMat4("model", BuildModelMatrix(glm::vec3(8.0f, 0.0f, 15.0f), icebergDRotation + glm::vec3(0, 10, 0), miniScale));
		icebergD->Draw(*activeShader);

		activeShader->setMat4("model", BuildModelMatrix(glm::vec3(12.0f, -0.05f, 15.0f), icebergDRotation + glm::vec3(0, 45, 0), miniScale));
		icebergD->Draw(*activeShader);

		// Fila 2 (Alineada con el centro del clúster)
		activeShader->setMat4("model", BuildModelMatrix(glm::vec3(8.5f, 0.0f, 18.0f), icebergDRotation + glm::vec3(0, -20, 0), miniScale));
		icebergD->Draw(*activeShader);

		activeShader->setMat4("model", BuildModelMatrix(glm::vec3(12.5f, -0.05f, 18.0f), icebergDRotation + glm::vec3(0, 75, 0), miniScale));
		icebergD->Draw(*activeShader);

		// Fila 3 (Alineada con la parte trasera del clúster)
		activeShader->setMat4("model", BuildModelMatrix(glm::vec3(8.0f, -0.1f, 21.0f), icebergDRotation + glm::vec3(0, 130, 0), miniScale));
		icebergD->Draw(*activeShader);

		activeShader->setMat4("model", BuildModelMatrix(glm::vec3(12.0f, 0.0f, 21.0f), icebergDRotation + glm::vec3(0, -5, 0), miniScale));
		icebergD->Draw(*activeShader);

		// =========================================================
		// Mosaicos dispersos (Tipo D - Lado opuesto al clúster)	
		// =========================================================


		glm::mat4 dFugitivo = BuildModelMatrix(glm::vec3(-8.0f, -0.05f, 35.0f), icebergDRotation + glm::vec3(0, -25, 0), glm::vec3(1.2f));
		activeShader->setMat4("model", dFugitivo);
		icebergD->Draw(*activeShader);

		glm::mat4 dMiniFragmento = BuildModelMatrix(glm::vec3(-11.0f, -0.02f, 36.0f), icebergDRotation, glm::vec3(0.6f));
		activeShader->setMat4("model", dMiniFragmento);
		icebergD->Draw(*activeShader);

		// Hielito A
		activeShader->setMat4("model", BuildModelMatrix(glm::vec3(-15.0f, 0.0f, 40.0f), icebergDRotation, glm::vec3(0.5f)));
		icebergD->Draw(*activeShader);

		// Hielito B
		activeShader->setMat4("model", BuildModelMatrix(glm::vec3(-18.0f, -0.05f, 42.0f), icebergDRotation, glm::vec3(0.7f)));
		icebergD->Draw(*activeShader);

		// Hielito C
		activeShader->setMat4("model", BuildModelMatrix(glm::vec3(-12.0f, 0.0f, 44.0f), icebergDRotation, glm::vec3(0.4f)));
		icebergD->Draw(*activeShader);

		// =========================================================
		// RELLENO DE HORIZONTE (Entre Iceberg Grande y Chico)
		// =========================================================
	
		// Mosaico de fondo 1 (Hacia la izquierda del centro)
		activeShader->setMat4("model", BuildModelMatrix(glm::vec3(-20.0f, -0.2f, -15.0f), icebergDRotation + glm::vec3(10, 0, 0), glm::vec3(1.0f)));
		icebergD->Draw(*activeShader);

		// Mosaico de fondo 2 (Casi al centro pero más al fondo)
		activeShader->setMat4("model", BuildModelMatrix(glm::vec3(0.0f, -0.1f, -25.0f), icebergDRotation + glm::vec3(-10, 0, 0), glm::vec3(1.5f)));
		icebergD->Draw(*activeShader);

		// Mosaico de fondo 3 (Hacia la derecha del centro)
		activeShader->setMat4("model", BuildModelMatrix(glm::vec3(20.0f, -0.2f, -18.0f), icebergDRotation + glm::vec3(-9, 0, 0), glm::vec3(1.2f)));
		icebergD->Draw(*activeShader);

		// Un hielito más pequeño para romper la simetría
		activeShader->setMat4("model", BuildModelMatrix(glm::vec3(5.0f, -0.1f, -10.0f), icebergDRotation + glm::vec3(-7, 0, 0), glm::vec3(1.5f)));
		icebergD->Draw(*activeShader);

		// =========================================================
		// HORIZONTE LEJANO (Despegados de los originales)
		// =========================================================

		// Gigante al fondo izquierda: Lo mandamos más a la izquierda (-85) y más al fondo (-100)
		activeShader->setMat4("model", BuildModelMatrix(glm::vec3(-85.0f, -4.0f, -100.0f), glm::vec3(0, 180, 0), glm::vec3(18.0f)));
		icebergGrande->Draw(*activeShader);

		// Gigante al fondo derecha: Lo mandamos más a la derecha (90) y un poco menos al fondo (-85)
		activeShader->setMat4("model", BuildModelMatrix(glm::vec3(90.0f, -3.5f, -85.0f), glm::vec3(0, 45, 0), glm::vec3(12.0f)));
		icebergChico->Draw(*activeShader);

		// =========================================================
		// RELLENO DE HORIZONTE PROFUNDO (Atrás de los Gigantes)
		// =========================================================

		// Mosaico de fondo 1 (Lejos a la izquierda y muy al fondo)
		activeShader->setMat4("model", BuildModelMatrix(glm::vec3(-50.0f, -0.2f, -120.0f), icebergDRotation + glm::vec3(10, 0, 0), glm::vec3(2.5f)));
		icebergD->Draw(*activeShader);

		// Mosaico de fondo 2 (Casi central pero al fondo del todo)
		activeShader->setMat4("model", BuildModelMatrix(glm::vec3(-10.0f, -0.1f, -140.0f), icebergDRotation + glm::vec3(-10, 0, 0), glm::vec3(3.0f)));
		icebergD->Draw(*activeShader);

		// Mosaico de fondo 3 (Hacia la derecha, entre los gigantes)
		activeShader->setMat4("model", BuildModelMatrix(glm::vec3(40.0f, -0.2f, -115.0f), icebergDRotation + glm::vec3(-9, 0, 0), glm::vec3(2.8f)));
		icebergD->Draw(*activeShader);

		// Un hielito más pequeño para la silueta del fondo
		activeShader->setMat4("model", BuildModelMatrix(glm::vec3(15.0f, -0.1f, -130.0f), icebergDRotation + glm::vec3(-7, 0, 0), glm::vec3(2.0f)));
		icebergD->Draw(*activeShader);

		// =========================================================
		// AGREGANDO ICEBERGS TIPO A (Elevaciones y Montañitas)
		// =========================================================


		// Posición: X = -15, Z = 20 | Escala: 3.5f
		activeShader->setMat4("model", BuildModelMatrix(glm::vec3(-15.0f, 0.1f, 20.0f), icebergARotation + glm::vec3(0, 45, 0), glm::vec3(3.5f)));
		icebergA->Draw(*activeShader);

		// 2. Uno pequeño junto a los mosaicos pequeños de la derecha
		// Posición: X = 18, Z = 16 | Escala: 2.0f
		activeShader->setMat4("model", BuildModelMatrix(glm::vec3(18.0f, 0.0f, 16.0f), icebergARotation + glm::vec3(0, -20, 0), glm::vec3(2.0f)));
		icebergA->Draw(*activeShader);

		// 3. Uno más "hundido" en el agua por el área del fondo
		// Posición: X = 5, Z = 45 | Escala: 4.0f
		activeShader->setMat4("model", BuildModelMatrix(glm::vec3(5.0f, -0.5f, 45.0f), icebergARotation + glm::vec3(0, 90, 0), glm::vec3(4.0f)));
		icebergA->Draw(*activeShader);

		// 4. Un par de "picos" lejanos para acompañar el horizonte profundo
		// Pico lejos izquierda
		activeShader->setMat4("model", BuildModelMatrix(glm::vec3(-40.0f, -0.2f, -90.0f), icebergARotation, glm::vec3(6.0f)));
		icebergA->Draw(*activeShader);

		// Pico lejos derecha
		activeShader->setMat4("model", BuildModelMatrix(glm::vec3(45.0f, -0.2f, -110.0f), icebergARotation + glm::vec3(0, 180, 0), glm::vec3(5.5f)));
		icebergA->Draw(*activeShader);

		// =========================================================
		// MICRO-IGLÚ 1
		// =========================================================

		// 1. Base de hielo (Tipo D) - Escala reducida a 2.0f
		glm::mat4 baseIglu1 = BuildModelMatrix(glm::vec3(-20.0f, 0.0f, 10.0f), icebergDRotation, glm::vec3(2.0f));
		activeShader->setMat4("model", baseIglu1);
		icebergD->Draw(*activeShader);

		// 2. El Iglú - Escala mini a 0.3f
		// Lo subimos apenas un poquito (Y=0.05) para que se asiente bien
		glm::mat4 iglu1ModelMat = BuildModelMatrix(glm::vec3(-20.0f, 0.05f, 10.0f), glm::vec3(-90.0f, 45.0f, 0.0f), glm::vec3(0.3f));
		activeShader->setMat4("model", iglu1ModelMat);
		iglu->Draw(*activeShader);


		// =========================================================
		// MICRO-IGLÚ 2 
		// =========================================================

		// 1. Base de hielo (Tipo D) - Escala 1.5f
		glm::mat4 baseIglu2 = BuildModelMatrix(glm::vec3(25.0f, -0.1f, -5.0f), icebergDRotation, glm::vec3(1.5f));
		activeShader->setMat4("model", baseIglu2);
		icebergD->Draw(*activeShader);

		// 2. El Iglú - Escala mini a 0.25f
		glm::mat4 iglu2ModelMat = BuildModelMatrix(glm::vec3(25.0f, 0.02f, -5.0f), glm::vec3(-90.0f, -30.0f, 0.0f), glm::vec3(0.25f));
		activeShader->setMat4("model", iglu2ModelMat);
		iglu->Draw(*activeShader);

		// =========================================================
		// HABITANTES DEL ÁRTICO (Animales en sus posiciones)
		// =========================================================

		//oso tipo a: Lo colocamos cerca del clúster
		activeShader->setMat4("model", BuildModelMatrix(glm::vec3(0.0f, 22.0f, 17.5f), osoARotation + glm::vec3(90, 180, 90), glm::vec3(1.2f)));
		osoA->Draw(*activeShader);

		//
		// Posición: X=8.0, Z=15.0 (encima de uno de los mosaicos pequeños de la derecha)
		// Altura: Y=0.1 para que pise el hielo plano
		// Rotación: -90 en X para acostarlo, y lo giramos un poco en Y para que vea al centro
		activeShader->setMat4("model", BuildModelMatrix(glm::vec3(15.0f, 13.0f, 15.0f), bearRotation + glm::vec3(-90.0f, -30.0f, 0.0f), glm::vec3(1.8f)));
		bear->Draw(*activeShader);

		// --- PINGUINO 1 ---

		// Posición: Cerca de X=-20, Z=10 | Escala: 0.15f (Mini para que combine con el iglú)
		activeShader->setMat4("model", BuildModelMatrix(glm::vec3(5.5f, 5.0f, -10.0f), pinguinoRotation + glm::vec3(-90.0f, 0.0f, 160.0f), glm::vec3(1.15f)));
		pinguino->Draw(*activeShader);

		// PINGUINO 2 
		// Posición: X=12.0, Z=21.0 | Escala: 0.18f
		activeShader->setMat4("model", BuildModelMatrix(glm::vec3(10.0f, 4.0f, -15.0f), pinguinoRotation + glm::vec3(-90.0f, 0.0f, 180.0f), glm::vec3(1.0f)));
		pinguino->Draw(*activeShader);

		//Orca 

		// Posición: X = 0 (centrada), Z = -15 (entre el centro y el fondo)
		// Altura: Y = -1.5f (Importante: valor negativo para que esté sumergida)
		// Rotación: -90 en X (para acostarla) y la giramos 90 en Y para que pase de lado
		
		// Calculamos el movimiento de balanceo
		float balanceo = sin(glfwGetTime() * 1.5f) * 0.5f; // Sube y baja 0.5 unidades
		float inclinacion = sin(glfwGetTime() * 0.8f) * 5.0f; // Se inclina 5 grados

		activeShader->setMat4("model", BuildModelMatrix(
			glm::vec3(10.0f, -6.0f + balanceo, -65.0f), // Aquí le sumamos el balanceo a Y
			orcaRotation + glm::vec3(-90.0f + inclinacion, 0.0f, 0.0f), // Aquí inclinamos en X
			glm::vec3(4.5f)
		));
		orca->Draw(*activeShader);

	
		// Lobo 1: Cerca del centro pero hacia la derecha (X = 10) y un poco más al fondo (Z = -125)
		activeShader->setMat4("model", BuildModelMatrix(
			glm::vec3(10.0f, 2.5f, -125.0f),
			wolfRotation + glm::vec3(-90.0f, 0.0f, 0.0f),
			glm::vec3(5.5f)
		));
		wolf->Draw(*activeShader);

		// Lobo 2: Más a la derecha (X = 34) y un poco más al fondo (Z = -128)
		activeShader->setMat4("model", BuildModelMatrix(glm::vec3(15.0f, 2.5f, -128.0f), wolfRotation + glm::vec3(-90.0f, 0.0f, 3.0f), glm::vec3(5.2f)));
		wolf->Draw(*activeShader);

		// Lobo 3: Más a la derecha (X = 34) y un poco más al fondo (Z = -128)
		activeShader->setMat4("model", BuildModelMatrix(glm::vec3(20.50f, 2.5f, -128.0f), wolfRotation + glm::vec3(-90.0f, 0.0f, -1.0f), glm::vec3(5.4f)));
		wolf->Draw(*activeShader);

		//Leon Marino 1
		activeShader->setMat4("model", BuildModelMatrix(
			glm::vec3(-50.0f, 4.95f, -130.0f),
			leonMarinoRotation + glm::vec3(-90.0f, 0.0f, 0.0f),
			glm::vec3(0.7f)
		));
		leonMarino->Draw(*activeShader);

		// León Marino 2:
		// Posición: X = -26, Z = -12 | Escala: 0.65f (un pelín más chico)
		activeShader->setMat4("model", BuildModelMatrix(
			glm::vec3(-10.0f, 5.05f, -130.0f),
			leonMarinoRotation + glm::vec3(-90.0f, 0.0f, 90.0f),
			glm::vec3(1.65f)
		));
		leonMarino->Draw(*activeShader);

		if (lightingMode == 3) {
			// Segundo pase: Fresnel sobre los icebergs, el resto queda en Phong
			glDepthFunc(GL_LEQUAL);

			fresnelShader->use();
			fresnelShader->setMat4("projection", projection);
			fresnelShader->setMat4("view", view);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_CUBE_MAP, mainCubeMap->textureID);
			fresnelShader->setInt("cubetex", 0);
			fresnelShader->setVec3("cameraPosition", eyePosition);
			fresnelShader->setFloat("mRefractionRatio", 1.0f / 1.31f);  // Hielo
			fresnelShader->setFloat("_Bias", 0.08f);
			fresnelShader->setFloat("_Scale", 0.22f);
			fresnelShader->setFloat("_Power", 2.0f);
			fresnelShader->setVec3("tintColor", glm::vec3(0.72f, 0.86f, 1.0f));
			fresnelShader->setFloat("tintStrength", 0.22f);

			fresnelShader->setMat4("model", icebergChicoModel);
			icebergChico->Draw(*fresnelShader);

			fresnelShader->setMat4("model", icebergGrandeModel);
			icebergGrande->Draw(*fresnelShader);

			fresnelShader->setMat4("model", BuildModelMatrix(glm::vec3(-20.0f, -0.2f, 15.0f), icebergDRotation, glm::vec3(2.5f)));
			icebergD->Draw(*fresnelShader);

			fresnelShader->setMat4("model", BuildModelMatrix(glm::vec3(0.0f, 0.2f, 17.5f), icebergARotation, glm::vec3(3.5f)));
			icebergA->Draw(*fresnelShader);

			glDepthFunc(GL_LESS);
		}

		if (!activeCamera) {
			activeShader->setMat4("model", personajePlayerModel);
			personaje->Draw(*activeShader);
		}

		/*
		activeShader->setMat4("model", oilPumpModel);
		oilPump->Draw(*activeShader);

		activeShader->setMat4("model", orcaModel);
		orca->Draw(*activeShader);

		activeShader->setMat4("model", pezModel);
		pez->Draw(*activeShader);

		

		activeShader->setMat4("model", pinguinoModel);
		pinguino->Draw(*activeShader);

		activeShader->setMat4("model", renoModel);
		reno->Draw(*activeShader);

		// Dibujamos también el reno del jugador en la posición controlada
		activeShader->setMat4("model", renoPlayerModel);
		reno->Draw(*activeShader);

		activeShader->setMat4("model", rigModel);
		rig->Draw(*activeShader);

		activeShader->setMat4("model", sealModel);
		seal->Draw(*activeShader);

		activeShader->setMat4("model", shipModel);
		ship->Draw(*activeShader);

		activeShader->setMat4("model", tanqueDerramadoModel);
		tanqueDerramado->Draw(*activeShader);

		activeShader->setMat4("model", tanqueGrandeModel);
		tanqueGrande->Draw(*activeShader);

		activeShader->setMat4("model", tanquesModel);
		tanques->Draw(*activeShader);

		activeShader->setMat4("model", titanicModel);
		titanic->Draw(*activeShader);

		activeShader->setMat4("model", wolfModel);
		wolf->Draw(*activeShader);

		/*activeShader->setMat4("model", trabajadorAnimadoModel);
		trabajadorAnimado->Draw(*activeShader);

		activeShader->setMat4("model", trabajadoraAnimadaModel);
		trabajadoraAnimada->Draw(*activeShader);
		
		*/
	}

	glUseProgram(0);
	
	// glfw: swap buffers 
	glfwSwapBuffers(window);
	glfwPollEvents();

	return true;
}

// Procesamos entradas del teclado
void processInput(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
	{
		if (activeCamera) camera.ProcessKeyboard(FORWARD, deltaTime);
		else {
			// Move player forward in third-person using same speed scaling as first-person
			float moveStep = camera.MovementSpeed * deltaTime;
			position = position + moveStep * forwardView;
			camera3rd.Front = forwardView;
			camera3rd.Position = position;
			camera3rd.Position.y += trdpersonHeightOffset;
			camera3rd.Position -= trdpersonOffset * forwardView;
		}
	}
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
	{
		if (activeCamera) camera.ProcessKeyboard(BACKWARD, deltaTime);
		else {
			float moveStep = camera.MovementSpeed * deltaTime;
			position = position - moveStep * forwardView;
			camera3rd.Front = forwardView;
			camera3rd.Position = position;
			camera3rd.Position.y += trdpersonHeightOffset;
			camera3rd.Position -= trdpersonOffset * forwardView;
		}
	}
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
	{
		if (activeCamera) camera.ProcessKeyboard(LEFT, deltaTime);
		else {
			// Strafe left for player (frame-rate independent)
			glm::vec3 right = glm::normalize(glm::cross(forwardView, glm::vec3(0.0f,1.0f,0.0f)));
			glm::vec3 left = -right;
			float moveStep = camera.MovementSpeed * deltaTime;
			position += moveStep * left;
			camera3rd.Position = position;
			camera3rd.Position.y += trdpersonHeightOffset;
			camera3rd.Position -= trdpersonOffset * forwardView;
		}
	}
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
	{
		if (activeCamera) camera.ProcessKeyboard(RIGHT, deltaTime);
		else {
			// Strafe right for player (frame-rate independent)
			glm::vec3 right = glm::normalize(glm::cross(forwardView, glm::vec3(0.0f,1.0f,0.0f)));
			float moveStep = camera.MovementSpeed * deltaTime;
			position += moveStep * right;
			camera3rd.Position = position;
			camera3rd.Position.y += trdpersonHeightOffset;
			camera3rd.Position -= trdpersonOffset * forwardView;
		}
	}
	if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS)
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	if (glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS)
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS)
		glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);

	if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS)
		SetSelectedModelIndex(0);
	if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS)
		SetSelectedModelIndex(1);
	if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS)
		SetSelectedModelIndex(2);
	if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS)
		SetSelectedModelIndex(3);
	if (glfwGetKey(window, GLFW_KEY_5) == GLFW_PRESS)
		SetSelectedModelIndex(4);

	if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS)
		TranslateSelectedModel(glm::vec3(-0.02f, 0.0f, 0.0f));
	if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS)
		TranslateSelectedModel(glm::vec3(0.02f, 0.0f, 0.0f));
	if (glfwGetKey(window, GLFW_KEY_Y) == GLFW_PRESS)
		TranslateSelectedModel(glm::vec3(0.0f, 0.02f, 0.0f));
	if (glfwGetKey(window, GLFW_KEY_H) == GLFW_PRESS)
		TranslateSelectedModel(glm::vec3(0.0f, -0.02f, 0.0f));
	if (glfwGetKey(window, GLFW_KEY_U) == GLFW_PRESS)
		TranslateSelectedModel(glm::vec3(0.0f, 0.0f, -0.02f));
	if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS)
		TranslateSelectedModel(glm::vec3(0.0f, 0.0f, 0.02f));
	if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS)
		RotateSelectedModel(glm::vec3(0.0f, -1.0f, 0.0f));
	if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS)
		RotateSelectedModel(glm::vec3(0.0f, 1.0f, 0.0f));
	if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS)
		ScaleSelectedModel(-0.01f);
	if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS)
		ScaleSelectedModel(0.01f);

	// Cambiar tipo de iluminación (L = Lighting)
	static bool lKeyPressed = false;
	if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS && !lKeyPressed) {
		lightingMode = (lightingMode + 1) % 4;
		lKeyPressed = true;
		const char* modeNames[] = { "Phong MultiLights", "Fresnel", "Phong Simple", "Phong + Fresnel" };
		std::cout << "Lighting mode changed to: " << modeNames[lightingMode] << std::endl;
	}
	if (glfwGetKey(window, GLFW_KEY_L) == GLFW_RELEASE) {
		lKeyPressed = false;
	}

	// Character movement
	if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {

		float moveStep = camera.MovementSpeed * deltaTime;
		position = position + moveStep * forwardView;
		camera3rd.Front = forwardView;
		camera3rd.ProcessKeyboard(FORWARD, deltaTime);
		camera3rd.Position = position;
		camera3rd.Position.y += trdpersonHeightOffset;
		camera3rd.Position -= trdpersonOffset * forwardView;

	}
	if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
		float moveStep = camera.MovementSpeed * deltaTime;
		position = position - moveStep * forwardView;
		camera3rd.Front = forwardView;
		camera3rd.ProcessKeyboard(BACKWARD, deltaTime);
		camera3rd.Position = position;
		camera3rd.Position.y += trdpersonHeightOffset;
		camera3rd.Position -= trdpersonOffset * forwardView;
	}
	if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
		// make rotation frame-rate independent and proportional to mouse sensitivity
		float rotationSpeed = camera.MouseSensitivity * 100.0f * deltaTime;
		rotateCharacter += rotationSpeed;

		glm::mat4 model = glm::mat4(1.0f);
		model = glm::rotate(model, glm::radians(rotateCharacter), glm::vec3(0.0f, 1.0f, 0.0f));
		glm::vec4 viewVector = model * glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);
		forwardView = glm::vec3(viewVector);
		forwardView = glm::normalize(forwardView);

		camera3rd.Front = forwardView;
		camera3rd.Position = position;
		camera3rd.Position.y += trdpersonHeightOffset;
		camera3rd.Position -= trdpersonOffset * forwardView;
	}
	if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
		float rotationSpeed = camera.MouseSensitivity * 100.0f * deltaTime;
		rotateCharacter -= rotationSpeed;

		glm::mat4 model = glm::mat4(1.0f);
		model = glm::rotate(model, glm::radians(rotateCharacter), glm::vec3(0.0f, 1.0f, 0.0f));
		glm::vec4 viewVector = model * glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);
		forwardView = glm::vec3(viewVector);
		forwardView = glm::normalize(forwardView);

		camera3rd.Front = forwardView;
		camera3rd.Position = position;
		camera3rd.Position.y += trdpersonHeightOffset;
		camera3rd.Position -= trdpersonOffset * forwardView;
	}

	if (glfwGetKey(window, GLFW_KEY_F1) == GLFW_PRESS)
		activeCamera = 0;
	if (glfwGetKey(window, GLFW_KEY_F2) == GLFW_PRESS)
		activeCamera = 1;
	
}

// glfw: Actualizamos el puerto de vista si hay cambios del tamaño
// de la ventana
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}

// glfw: Callback del movimiento y eventos del mouse
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	if (firstMouse)
	{
		lastX = (float)xpos;
		lastY = (float)ypos;
		firstMouse = false;
	}

	float xoffset = (float)xpos - lastX;
	float yoffset = lastY - (float)ypos; 

	lastX = (float)xpos;
	lastY = (float)ypos;

	if (activeCamera) {
		camera.ProcessMouseMovement(xoffset, yoffset);
	}
	else {
		// Rotate the character horizontally with mouse in 3rd person
		// Use same effective sensitivity as the Camera class
		rotateCharacter += xoffset * camera.MouseSensitivity;

		glm::mat4 model = glm::mat4(1.0f);
		model = glm::rotate(model, glm::radians(rotateCharacter), glm::vec3(0.0f, 1.0f, 0.0f));
		glm::vec4 viewVector = model * glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);
		forwardView = glm::vec3(viewVector);
		forwardView = glm::normalize(forwardView);

		camera3rd.Front = forwardView;
		camera3rd.Position = position;
		camera3rd.Position.y += trdpersonHeightOffset;
		camera3rd.Position -= trdpersonOffset * forwardView;
	}
}

// glfw: Complemento para el movimiento y eventos del mouse
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	camera.ProcessMouseScroll((float)yoffset);
}
