/*
* 
* 15 - Proyecto final
*/

#include <iostream>
#include <stdlib.h>
#include <vector>
#include <algorithm>

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
void InitPollutionGameplay();
void ResetPollutionGameplay();
void UpdatePollutionGameplay();
void DrawActiveOilPumps(Shader& shader);
void DrawSceneIcebergs(Shader& shader, float derretimiento);
void DrawSceneIcebergsFresnel(Shader& shader, float derretimiento);

// Definición de callbacks
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);

// Gobals
GLFWwindow* window;
// 0 = Saludable, 1 = Contaminación inicial, 2 = Desastre industrial
int sceneMode = 0;

Material material01;

// --- Sistema dinámico de contaminación (extractores + derretimiento de icebergs) ---
const float kOilPumpSpawnInterval = 10.0f;
const float kIcebergMeltInterval = 10.0f;
const int   kPumpsForTransition = 3;
const int   kPumpsForDisaster = 8; // 3 + 5 adicionales
const float kOilPumpTouchRadius = 7.5f;
static const glm::vec3 kOilPumpDrawRotation(0.0f, 0.0f, 0.0f);

struct PlacedOilPump {
	glm::vec3 position;
	glm::vec3 rotation;
	glm::vec3 scale;
};

std::vector<PlacedOilPump> activeOilPumps;
std::vector<bool> icebergSlotVisible;
float oilPumpSpawnTimer = 0.0f;
float icebergMeltTimer = 0.0f;
int oilPumpSpawnCursor = 0;
bool pollutionGameplayInitialized = false;

static const glm::vec3 kOilPumpSpawnPoints[] = {
	glm::vec3(30.0f, 0.5f, 10.0f),
	glm::vec3(-35.0f, 0.5f, 8.0f),
	glm::vec3(45.0f, 0.5f, -8.0f),
	glm::vec3(-45.0f, 0.5f, -5.0f),
	glm::vec3(5.0f, 0.5f, 28.0f),
	glm::vec3(-10.0f, 0.5f, -18.0f),
	glm::vec3(20.0f, 0.5f, -22.0f),
	glm::vec3(-25.0f, 0.5f, 22.0f),
	glm::vec3(55.0f, 0.5f, 5.0f),
	glm::vec3(-55.0f, 0.5f, 12.0f),
	glm::vec3(0.0f, 0.5f, -30.0f),
	glm::vec3(15.0f, 0.5f, 18.0f),
};
static const int kOilPumpSpawnPointCount = (int)(sizeof(kOilPumpSpawnPoints) / sizeof(kOilPumpSpawnPoints[0]));

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
	float     trdpersonOffset = 10.0f;
	// Subir altura de la cámara en 4 unidades
	float     trdpersonHeightOffset = 6.0f;
	// Altura del personaje 
	float     trdpersonCharacterYOffset = 4.3f;
	// Escala del jugador en 3ª persona (Mixamo + skinning; sube si es muy pequeño)
	glm::vec3 playerCharacterScale(0.01f, 0.01f, 0.01f);
	// El FBX del ave mira hacia la cámara; +180° en Y alinea espalda con la dirección de marcha (ajusta si tu export es distinto)
	const float playerCharacterYawOffsetDeg = 180.0f;
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

glm::vec3 barconewPosition(6.0f, 5.0f, -190.0f);
glm::vec3 icebergChicoPosition(60.0f, -2.0f, -20.0f);
glm::vec3 icebergGrandePosition(-45.0f, -1.0f, -40.0f);
glm::vec3 igluPosition(-4.8f, 0.0f, -3.5f);
glm::vec3 zorroPosition(1.8f, 0.0f, -5.6f);
glm::vec3 olaPosition(0.0f, 0.0f, 0.0f);

glm::vec3 barconewRotation(-90.0f, 0.0f, 90.0f);
glm::vec3 icebergChicoRotation(0.0f, 0.0f, 0.0f);
glm::vec3 icebergGrandeRotation(0.0f, 0.0f, 0.0f);
glm::vec3 igluRotation(0.0f, 0.0f, 0.0f);
glm::vec3 zorroRotation(0.0f, 0.0f, 0.0f);
glm::vec3 olaRotation(-90.0f, 0.0f, 0.0f);

glm::vec3 barconewScale(5.0f, 5.0f, 5.0f);
glm::vec3 icebergChicoScale(25.0f, 25.0f, 25.0f);
glm::vec3 icebergGrandeScale(12.0f, 12.0f, 12.0f);
glm::vec3 igluScale(1.0f, 1.0f, 1.0f);
glm::vec3 zorroScale(1.0f, 1.0f, 1.0f);
glm::vec3 olaScale(50.0f, 50.0f, 50.0f);

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
AnimatedModel* trabajadorAnimado;
AnimatedModel* trabajadoraAnimada;
AnimatedModel* personaje;

glm::vec3 leonMarinoPosition(-18.0f, 0.0f, 12.0f);
glm::vec3 osoAPosition(-12.0f, 0.0f, 12.0f);
glm::vec3 pexDoradoPosition(27.0f, 5.0f, -40.0f);
glm::vec3 bearPosition(0.0f, 0.0f, 12.0f);
glm::vec3 cargoPosition(170.0f, 2.0f, 12.0f);
glm::vec3 fishPosition(0.0f, 20.0f, 0.0f);
glm::vec3 gasPosition(-18.0f, 0.0f, 6.0f);
glm::vec3 icebergAPosition(-25.0f, -1.0f, 25.0f);
glm::vec3 icebergDPosition(15.0f, 0.0f, 15.0f);
glm::vec3 oilPumpPosition(-150.0f, 0.5f, 6.0f);
glm::vec3 orcaPosition(6.0f, 0.0f, 6.0f);
glm::vec3 pezPosition(-40.0f, 1.0f,10.0f);
glm::vec3 pinguinoPosition(-18.0f, 0.0f, 0.0f);
glm::vec3 renoPosition(-12.0f, 0.0f, 0.0f);
glm::vec3 rigPosition(-6.0f, -5.0f, 0.0f);
glm::vec3 sealPosition(0.0f, 0.0f, 0.0f);
glm::vec3 shipPosition(6.0f, 0.0f, 0.0f);
glm::vec3 tanqueDerramadoPosition(12.0f, 0.0f, 0.0f);
glm::vec3 tanqueGrandePosition(-150.0f, 6.0f, 10.0f);
glm::vec3 tanquesPosition(-12.0f, 0.0f, -6.0f);
glm::vec3 titanicPosition(-6.0f, 20.0f, -6.0f);
glm::vec3 wolfPosition(0.0f, 0.0f, -6.0f);
glm::vec3 trabajadorAnimadoPosition(24.0f, 3.0f, -6.0f);
glm::vec3 trabajadoraAnimadaPosition(51.0f, 10.0f, -6.0f);

glm::vec3 leonMarinoRotation(0.0f, 0.0f, 0.0f);
glm::vec3 osoARotation(0.0f, 0.0f, 0.0f);
glm::vec3 pexDoradoRotation(-45.0f, 0.0f, 0.0f);
glm::vec3 bearRotation(0.0f, 0.0f, 0.0f);
glm::vec3 cargoRotation(-90.0f, 0.0f, 65.0f);
glm::vec3 fishRotation(0.0f, 0.0f, 0.0f);
glm::vec3 gasRotation(0.0f, 0.0f, 0.0f);
glm::vec3 icebergARotation(-90.0f, 0.0f, 0.0f);
glm::vec3 icebergDRotation(-90.0f, 0.0f, 0.0f);
glm::vec3 oilPumpRotation(-90.0f, 0.0f, -90.0f);
glm::vec3 orcaRotation(0.0f, 0.0f, 0.0f);
glm::vec3 pezRotation(0.0f, -120.0f, 0.0f);
glm::vec3 pinguinoRotation(0.0f, 0.0f, 0.0f);
glm::vec3 renoRotation(0.0f, 0.0f, 0.0f);
glm::vec3 rigRotation(0.0f, 0.0f, 0.0f);
glm::vec3 sealRotation(0.0f, 0.0f, 0.0f);
glm::vec3 shipRotation(0.0f, 0.0f, 0.0f);
glm::vec3 tanqueDerramadoRotation(0.0f, 0.0f, 0.0f);
glm::vec3 tanqueGrandeRotation(-90.0f, 0.0f, 90.0f);
glm::vec3 tanquesRotation(0.0f, 0.0f, 0.0f);
glm::vec3 titanicRotation(0.0f, 0.0f, 0.0f);
glm::vec3 wolfRotation(0.0f, 0.0f, 0.0f);
glm::vec3 trabajadorAnimadoRotation(0.0f, 0.0f, 0.0f);
glm::vec3 trabajadoraAnimadaRotation(0.0f, 90.0f, 0.0f);

bool wolfCelebrationActive = false;
float wolfCelebrationTime = 0.0f;
const float wolfCelebrationDuration = 0.9f;
const float wolfCelebrationAngle = 8.0f;

bool pinguinoDeathTargetDown = false;
float pinguinoDeathOffsetY = 0.0f;
const float pinguinoDeathOffsetDown = -20.0f;
const float pinguinoDeathSlideSpeed = 5.0f;

bool zorroBreathingActive = false;
float zorroBreathingTime = 0.0f;
float zorroBreathingScaleFactor = 1.0f;
const float zorroBreathingCycleDuration = 0.32f;
const int zorroBreathingCycles = 4;
const float zorroBreathingScaleAmount = 0.035f;

glm::vec3 leonMarinoScale(1.0f, 1.0f, 1.0f);
glm::vec3 osoAScale(1.0f, 1.0f, 1.0f);
glm::vec3 pexDoradoScale(1.0f, 1.0f, 1.0f);
glm::vec3 bearScale(1.0f, 1.0f, 1.0f);
glm::vec3 cargoScale(1.0f, 1.0f, 1.0f);
glm::vec3 fishScale(1.0f, 1.0f, 1.0f);
glm::vec3 gasScale(1.0f, 1.0f, 1.0f);
glm::vec3 icebergAScale(5.0f, 5.0f, 5.0f);
glm::vec3 icebergDScale(1.0f, 1.0f, 1.0f);
glm::vec3 oilPumpScale(4.5f, 4.5f, 4.5f);
glm::vec3 orcaScale(1.0f, 1.0f, 1.0f);
glm::vec3 pezScale(0.01f, 0.01f, 0.01f);
glm::vec3 pinguinoScale(1.0f, 1.0f, 1.0f);
glm::vec3 renoScale(1.0f, 1.0f, 1.0f);
glm::vec3 rigScale(1.0f, 1.0f, 1.0f);
glm::vec3 sealScale(1.0f, 1.0f, 1.0f);
glm::vec3 shipScale(1.0f, 1.0f, 1.0f);
glm::vec3 tanqueDerramadoScale(1.0f, 1.0f, 1.0f);
glm::vec3 tanqueGrandeScale(4.0f, 4.0f, 4.0f);
glm::vec3 tanquesScale(1.0f, 1.0f, 1.0f);
glm::vec3 titanicScale(0.1f, 0.1f, 0.1f);
glm::vec3 wolfScale(1.0f, 1.0f, 1.0f);
glm::vec3 trabajadorAnimadoScale(0.035f, 0.035f, 0.035f);
glm::vec3 trabajadoraAnimadaScale(0.035f, 0.035f, 0.035f);

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

void InitPollutionGameplay() {
	icebergSlotVisible.assign(36, true);
	activeOilPumps.clear();
	oilPumpSpawnTimer = 0.0f;
	icebergMeltTimer = 0.0f;
	oilPumpSpawnCursor = 0;
	sceneMode = 0;
	pollutionGameplayInitialized = true;
}

void ResetPollutionGameplay() {
	InitPollutionGameplay();
	std::cout << "Ecosistema reiniciado (sin extractores)." << std::endl;
}

static void UpdateSceneModeFromPumpCount() {
	const int count = (int)activeOilPumps.size();
	if (count >= kPumpsForDisaster) {
		sceneMode = 2;
	}
	else if (count >= kPumpsForTransition) {
		sceneMode = 1;
	}
	else {
		sceneMode = 0;
	}
}

static void SpawnOilPump() {
	PlacedOilPump pump;
	pump.position = kOilPumpSpawnPoints[oilPumpSpawnCursor % kOilPumpSpawnPointCount];
	pump.rotation = kOilPumpDrawRotation;
	pump.scale = oilPumpScale;
	activeOilPumps.push_back(pump);
	oilPumpSpawnCursor++;
	std::cout << "Nuevo extractor de petroleo (" << activeOilPumps.size() << " activos)." << std::endl;
	UpdateSceneModeFromPumpCount();
}

static void RemoveOilPumpsPlayerTouch() {
	glm::vec3 playerXZ(position.x, 0.0f, position.z);
	bool removed = false;
	for (auto it = activeOilPumps.begin(); it != activeOilPumps.end(); ) {
		glm::vec3 pumpXZ(it->position.x, 0.0f, it->position.z);
		if (glm::length(playerXZ - pumpXZ) <= kOilPumpTouchRadius) {
			it = activeOilPumps.erase(it);
			removed = true;
		}
		else {
			++it;
		}
	}
	if (removed) {
		std::cout << "Extractor eliminado por el jugador. Restantes: " << activeOilPumps.size() << std::endl;
		UpdateSceneModeFromPumpCount();
		if (sceneMode < 2) {
			icebergMeltTimer = 0.0f;
			std::fill(icebergSlotVisible.begin(), icebergSlotVisible.end(), true);
		}
	}
}

static void HideNextIcebergSlot() {
	for (size_t i = 0; i < icebergSlotVisible.size(); ++i) {
		if (icebergSlotVisible[i]) {
			icebergSlotVisible[i] = false;
			std::cout << "Iceberg #" << (i + 1) << " desaparecio por derretimiento." << std::endl;
			return;
		}
	}
}

void UpdatePollutionGameplay() {
	if (!pollutionGameplayInitialized) {
		InitPollutionGameplay();
	}

	RemoveOilPumpsPlayerTouch();

	if (sceneMode < 2) {
		oilPumpSpawnTimer += deltaTime;
		if (oilPumpSpawnTimer >= kOilPumpSpawnInterval) {
			oilPumpSpawnTimer = 0.0f;
			SpawnOilPump();
		}
	}

	if (sceneMode == 2) {
		icebergMeltTimer += deltaTime;
		if (icebergMeltTimer >= kIcebergMeltInterval) {
			icebergMeltTimer = 0.0f;
			HideNextIcebergSlot();
		}
	}
	else {
		icebergMeltTimer = 0.0f;
	}
}

void DrawActiveOilPumps(Shader& shader) {
	for (const PlacedOilPump& pump : activeOilPumps) {
		shader.setMat4("model", BuildModelMatrix(pump.position, pump.rotation, pump.scale));
		oilPump->Draw(shader);
	}
}

void DrawSceneIcebergs(Shader& shader, float derretimiento) {
	int slot = 0;
	auto drawSlot = [&](const glm::mat4& modelMat, Model* mesh) {
		if (slot < (int)icebergSlotVisible.size() && icebergSlotVisible[slot]) {
			shader.setMat4("model", modelMat);
			mesh->Draw(shader);
		}
		slot++;
	};

	drawSlot(icebergChicoModel, icebergChico);
	drawSlot(icebergGrandeModel, icebergGrande);

	drawSlot(BuildModelMatrix(icebergDPosition + glm::vec3(-2.0f, derretimiento, 15.0f), icebergDRotation, glm::vec3(2.5f)), icebergD);
	drawSlot(BuildModelMatrix(icebergDPosition + glm::vec3(2.0f, derretimiento * -0.1f, 15.5f), icebergDRotation + glm::vec3(0, 15, 0), glm::vec3(2.2f)), icebergD);
	drawSlot(BuildModelMatrix(icebergDPosition + glm::vec3(-2.5f, derretimiento * -0.05f, 19.0f), icebergDRotation + glm::vec3(0, -10, 0), glm::vec3(2.8f)), icebergD);
	drawSlot(BuildModelMatrix(icebergDPosition + glm::vec3(2.5f, derretimiento * 0.0f, 19.5f), icebergDRotation + glm::vec3(0, 45, 0), glm::vec3(2.3f)), icebergD);

	drawSlot(BuildModelMatrix(glm::vec3(0.0f, derretimiento * 0.2f, 17.5f), icebergARotation, glm::vec3(3.5f)), icebergA);

	glm::vec3 miniScale(1.5f);
	drawSlot(BuildModelMatrix(icebergDPosition + glm::vec3(8.0f, derretimiento * 0.0f, 15.0f), icebergDRotation + glm::vec3(0, 10, 0), miniScale), icebergD);
	drawSlot(BuildModelMatrix(icebergDPosition + glm::vec3(12.0f, derretimiento * -0.05f, 15.0f), icebergDRotation + glm::vec3(0, 45, 0), miniScale), icebergD);
	drawSlot(BuildModelMatrix(icebergDPosition + glm::vec3(8.5f, derretimiento * 0.0f, 18.0f), icebergDRotation + glm::vec3(0, -20, 0), miniScale), icebergD);
	drawSlot(BuildModelMatrix(icebergDPosition + glm::vec3(12.5f, derretimiento * -0.05f, 18.0f), icebergDRotation + glm::vec3(0, 75, 0), miniScale), icebergD);
	drawSlot(BuildModelMatrix(icebergDPosition + glm::vec3(8.0f, derretimiento * -0.1f, 21.0f), icebergDRotation + glm::vec3(0, 130, 0), miniScale), icebergD);
	drawSlot(BuildModelMatrix(icebergDPosition + glm::vec3(12.0f, derretimiento * 0.0f, 21.0f), icebergDRotation + glm::vec3(0, -5, 0), miniScale), icebergD);

	drawSlot(BuildModelMatrix(icebergDPosition + glm::vec3(-8.0f, derretimiento * -0.05f, 35.0f), icebergDRotation + glm::vec3(0, -25, 0), glm::vec3(1.2f)), icebergD);
	drawSlot(BuildModelMatrix(icebergDPosition + glm::vec3(-11.0f, derretimiento * -0.02f, 36.0f), icebergDRotation, glm::vec3(0.6f)), icebergD);
	drawSlot(BuildModelMatrix(icebergDPosition + glm::vec3(-15.0f, derretimiento * 0.0f, 40.0f), icebergDRotation, glm::vec3(0.5f)), icebergD);
	drawSlot(BuildModelMatrix(glm::vec3(-18.0f, derretimiento * -0.05f, 42.0f), icebergDRotation, glm::vec3(0.7f)), icebergD);
	drawSlot(BuildModelMatrix(glm::vec3(-12.0f, derretimiento * 0.0f, 44.0f), icebergDRotation, glm::vec3(0.4f)), icebergD);

	drawSlot(BuildModelMatrix(glm::vec3(-20.0f, derretimiento * -0.2f, -15.0f), icebergDRotation + glm::vec3(10, 0, 0), glm::vec3(1.0f)), icebergD);
	drawSlot(BuildModelMatrix(glm::vec3(0.0f, derretimiento * -0.1f, -25.0f), icebergDRotation + glm::vec3(-10, 0, 0), glm::vec3(1.5f)), icebergD);
	drawSlot(BuildModelMatrix(glm::vec3(20.0f, derretimiento * -0.2f, -18.0f), icebergDRotation + glm::vec3(-9, 0, 0), glm::vec3(1.2f)), icebergD);
	drawSlot(BuildModelMatrix(glm::vec3(5.0f, derretimiento * -0.1f, -10.0f), icebergDRotation + glm::vec3(-7, 0, 0), glm::vec3(1.5f)), icebergD);

	drawSlot(BuildModelMatrix(glm::vec3(-85.0f, derretimiento * -4.0f, -100.0f), glm::vec3(0, 180, 0), glm::vec3(18.0f)), icebergGrande);
	drawSlot(BuildModelMatrix(glm::vec3(90.0f, derretimiento * -3.5f, -85.0f), glm::vec3(0, 45, 0), glm::vec3(12.0f)), icebergChico);

	drawSlot(BuildModelMatrix(glm::vec3(-50.0f, derretimiento * -0.2f, -120.0f), icebergDRotation + glm::vec3(10, 0, 0), glm::vec3(2.5f)), icebergD);
	drawSlot(BuildModelMatrix(glm::vec3(-10.0f, derretimiento * -0.1f, -140.0f), icebergDRotation + glm::vec3(-10, 0, 0), glm::vec3(3.0f)), icebergD);
	drawSlot(BuildModelMatrix(glm::vec3(40.0f, derretimiento * -0.2f, -115.0f), icebergDRotation + glm::vec3(-9, 0, 0), glm::vec3(2.8f)), icebergD);
	drawSlot(BuildModelMatrix(glm::vec3(15.0f, derretimiento * -0.1f, -130.0f), icebergDRotation + glm::vec3(-7, 0, 0), glm::vec3(2.0f)), icebergD);

	drawSlot(BuildModelMatrix(glm::vec3(-15.0f, derretimiento * 0.1f, 20.0f), icebergARotation + glm::vec3(0, 45, 0), glm::vec3(3.5f)), icebergA);
	drawSlot(BuildModelMatrix(glm::vec3(18.0f, derretimiento * 0.0f, 16.0f), icebergARotation + glm::vec3(0, -20, 0), glm::vec3(2.0f)), icebergA);
	drawSlot(BuildModelMatrix(glm::vec3(5.0f, derretimiento * -0.5f, 45.0f), icebergARotation + glm::vec3(0, 90, 0), glm::vec3(4.0f)), icebergA);
	drawSlot(BuildModelMatrix(glm::vec3(-40.0f, derretimiento * -0.2f, -90.0f), icebergARotation, glm::vec3(6.0f)), icebergA);
	drawSlot(BuildModelMatrix(glm::vec3(45.0f, derretimiento * -0.2f, -110.0f), icebergARotation + glm::vec3(0, 180, 0), glm::vec3(5.5f)), icebergA);

	drawSlot(BuildModelMatrix(glm::vec3(-20.0f, derretimiento * 0.0f, 10.0f), icebergDRotation, glm::vec3(2.0f)), icebergD);
	drawSlot(BuildModelMatrix(glm::vec3(25.0f, derretimiento * -0.1f, -5.0f), icebergDRotation, glm::vec3(1.5f)), icebergD);

	if (slot < (int)icebergSlotVisible.size() && icebergSlotVisible[slot]) {
		if (sceneMode < 2) {
			shader.setMat4("model", icebergGrandeModel);
			icebergGrande->Draw(shader);
		}
		else {
			glm::mat4 derretido = BuildModelMatrix(icebergGrandePosition + glm::vec3(0, -5, 0), icebergGrandeRotation, icebergGrandeScale * 0.5f);
			shader.setMat4("model", derretido);
			icebergGrande->Draw(shader);
		}
	}
	slot++;
}

void DrawSceneIcebergsFresnel(Shader& shader, float derretimiento) {
	if (icebergSlotVisible.size() > 0 && icebergSlotVisible[0]) {
		shader.setMat4("model", icebergChicoModel);
		icebergChico->Draw(shader);
	}
	if (icebergSlotVisible.size() > 1 && icebergSlotVisible[1]) {
		shader.setMat4("model", icebergGrandeModel);
		icebergGrande->Draw(shader);
	}
	shader.setMat4("model", BuildModelMatrix(glm::vec3(-20.0f, -0.2f, 15.0f), icebergDRotation, glm::vec3(2.5f)));
	icebergD->Draw(shader);
	shader.setMat4("model", BuildModelMatrix(glm::vec3(0.0f, 0.2f, 17.5f), icebergARotation, glm::vec3(3.5f)));
	icebergA->Draw(shader);
}

void StartWolfCelebration() {
	wolfCelebrationActive = true;
	wolfCelebrationTime = 0.0f;
	wolfRotation = glm::vec3(0.0f, 0.0f, 0.0f);
}

void UpdateWolfCelebration() {
	if (!wolfCelebrationActive) {
		return;
	}

	wolfCelebrationTime += deltaTime;
	float progress = wolfCelebrationTime / wolfCelebrationDuration;

	if (progress >= 1.0f) {
		wolfRotation = glm::vec3(0.0f, 0.0f, 0.0f);
		wolfCelebrationActive = false;
		return;
	}

	float celebrationYaw = 0.0f;
	if (progress < 0.33f) {
		celebrationYaw = glm::mix(0.0f, -wolfCelebrationAngle, progress / 0.33f);
	} else if (progress < 0.66f) {
		celebrationYaw = glm::mix(-wolfCelebrationAngle, wolfCelebrationAngle, (progress - 0.33f) / 0.33f);
	} else {
		celebrationYaw = glm::mix(wolfCelebrationAngle, 0.0f, (progress - 0.66f) / 0.34f);
	}

	wolfRotation = glm::vec3(0.0f, celebrationYaw, 0.0f);
}

glm::vec3 GetPinguinoRenderPosition() {
	return pinguinoPosition + glm::vec3(0.0f, pinguinoDeathOffsetY, 0.0f);
}

void UpdatePinguinoDeath() {
	float targetOffsetY = pinguinoDeathTargetDown ? pinguinoDeathOffsetDown : 0.0f;
	float slideStep = pinguinoDeathSlideSpeed * deltaTime;
	pinguinoDeathOffsetY = glm::mix(pinguinoDeathOffsetY, targetOffsetY, glm::clamp(slideStep, 0.0f, 1.0f));

	if (glm::abs(pinguinoDeathOffsetY - targetOffsetY) < 0.01f) {
		pinguinoDeathOffsetY = targetOffsetY;
	}
}

void StartZorroBreathing() {
	zorroBreathingActive = true;
	zorroBreathingTime = 0.0f;
	zorroBreathingScaleFactor = 1.0f;
}

void UpdateZorroBreathing() {
	if (!zorroBreathingActive) {
		zorroBreathingScaleFactor = 1.0f;
		return;
	}

	zorroBreathingTime += deltaTime;
	float totalDuration = zorroBreathingCycleDuration * static_cast<float>(zorroBreathingCycles);
	if (zorroBreathingTime >= totalDuration) {
		zorroBreathingActive = false;
		zorroBreathingScaleFactor = 1.0f;
		return;
	}

	float cycleTime = fmod(zorroBreathingTime, zorroBreathingCycleDuration);
	float halfCycle = zorroBreathingCycleDuration * 0.5f;
	float pulseAmount = 0.0f;
	if (cycleTime < halfCycle) {
		pulseAmount = cycleTime / halfCycle;
	} else {
		pulseAmount = 1.0f - ((cycleTime - halfCycle) / halfCycle);
	}

	zorroBreathingScaleFactor = 1.0f + (zorroBreathingScaleAmount * glm::clamp(pulseAmount, 0.0f, 1.0f));
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
	zorroModel = BuildModelMatrix(zorroPosition, zorroRotation, zorroScale * glm::vec3(zorroBreathingScaleFactor));
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
	pinguinoModel = BuildModelMatrix(GetPinguinoRenderPosition(), pinguinoRotation, pinguinoScale);
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
	personajePlayerModel = BuildModelMatrix(position + glm::vec3(0.0f, trdpersonCharacterYOffset, 0.0f), glm::vec3(0.0f, rotateCharacter + playerCharacterYawOffsetDeg, 0.0f), playerCharacterScale);
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

float proceduralTime = 0.0f;
float wavesTime = 0.0f;
bool waveAnimationActive = true; // Toggle para animación de la ola

// Audio
ISoundEngine *SoundEngine = createIrrKlangDevice();

// selección de cámara
bool    activeCamera = 0; // 0 = tercera persona (se dibuja el jugador); F2 = primera persona

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

	std::cout << "Attempting to load: models/Bird flying.fbx" << std::endl;
	personaje = new AnimatedModel("models/Bird flying.fbx");
	loadedModels++;

	std::cout << "Attempting to load: models/trabajadores/Trabajador.fbx (animado)" << std::endl;
	trabajadorAnimado = new AnimatedModel("models/trabajadores/Trabajador.fbx");
	loadedModels++;

	std::cout << "Attempting to load: models/trabajadores/Trabajadora.fbx (animada)" << std::endl;
	trabajadoraAnimada = new AnimatedModel("models/trabajadores/Trabajadora.fbx");
	loadedModels++;

	std::cout << "Attempting to load: models/rig.fbx" << std::endl;
	rig = new Model("models/rig.fbx");
	loadedModels++;

	std::cout << "Attempting to load: models/seal.fbx" << std::endl;
	seal = new Model("models/seal.fbx");
	loadedModels++;

	std::cout << "Loaded " << loadedModels << " individual models" << std::endl;

	InitPollutionGameplay();

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
	light01.Position = glm::vec3(5.0f, 3.0f, 5.0f);
	light01.Color = glm::vec4(0.2f, 0.2f, 0.2f, 1.0f);
	gLights.push_back(light01);

	Light light02;
	light02.Position = glm::vec3(-5.0f, 3.0f, 5.0f);
	light02.Color = glm::vec4(0.2f, 0.2f, 0.2f, 1.0f);
	gLights.push_back(light02);

	Light light03;
	light03.Position = glm::vec3(5.0f, 3.0f, -5.0f);
	light03.Color = glm::vec4(0.15f, 0.15f, 0.15f, 1.0f);
	gLights.push_back(light03);

	Light light04;
	light04.Position = glm::vec3(-5.0f, 3.0f, -5.0f);
	light04.Color = glm::vec4(0.15f, 0.15f, 0.15f, 1.0f);
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
	float derretimiento = 1.0f;
	if (sceneMode == 1) derretimiento = 2.0f; // Se hunden un poco
	if (sceneMode == 2) derretimiento = 6.0f; // Se hunden mucho (derretidos)

	// Cálculo del framerate
	float currentFrame = (float)glfwGetTime();
	deltaTime = currentFrame - lastFrame;
	lastFrame = currentFrame;

	// Procesa la entrada del teclado o mouse
	processInput(window);
	UpdateWolfCelebration();
	UpdatePinguinoDeath();
	UpdateZorroBreathing();
	UpdatePollutionGameplay();

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

		DrawSceneIcebergs(*activeShader, derretimiento);
		DrawActiveOilPumps(*activeShader);

		// Draw the wave mesh using the waves shader (procedural animation)
		wavesShader->use();

		// Activamos para objetos transparentes
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		wavesShader->setMat4("projection", projection);
		wavesShader->setMat4("view", view);
		wavesShader->setMat4("model", olaModel);

		// El color del agua cambia según el estado del ecosistema (F5/F6/F7)
		glm::vec3 waveDarkColor(0.10f, 0.40f, 0.80f);
		glm::vec3 waveLightColor(0.30f, 0.70f, 1.00f);
		if (sceneMode == 1) {
			waveDarkColor = glm::vec3(0.06f, 0.24f, 0.50f);
			waveLightColor = glm::vec3(0.16f, 0.45f, 0.75f);
		}
		else if (sceneMode == 2) {
			waveDarkColor = glm::vec3(0.02f, 0.05f, 0.08f);
			waveLightColor = glm::vec3(0.08f, 0.14f, 0.20f);
		}
		wavesShader->setVec3("waterColorDark", waveDarkColor);
		wavesShader->setVec3("waterColorLight", waveLightColor);

		// parámatros de la ola
		wavesShader->setFloat("time", wavesTime);
		wavesShader->setFloat("radius", 2.0f);
		wavesShader->setFloat("height", 3.0f); // amplitud del oleaje (menor = olas más bajas)

		ola->Draw(*wavesShader);
		
		// Actualizar tiempo de animación solo si la ola está animada
		if (waveAnimationActive) {
			wavesTime += 0.017f; // avance del tiempo de animación (menor = movimiento más lento)
		}

		// restore previously active shader
		activeShader->use();

		/*

		/*
		activeShader->setMat4("model", igluModel);
		iglu->Draw(*activeShader);

		activeShader->setMat4("model", zorroModel);
		zorro->Draw(*activeShader);

		
		activeShader->setMat4("model", leonMarinoModel);
		leonMarino->Draw(*activeShader);

		activeShader->setMat4("model", osoAModel);
		osoA->Draw(*activeShader);

		

		activeShader->setMat4("model", bearModel);
		bear->Draw(*activeShader);

		

		activeShader->setMat4("model", gasModel);
		gas->Draw(*activeShader);*/

		activeShader->setMat4("model", BuildModelMatrix(glm::vec3(-20.0f, 0.05f, 10.0f), glm::vec3(-90.0f, 45.0f, 0.0f), glm::vec3(0.3f)));
		iglu->Draw(*activeShader);
		activeShader->setMat4("model", BuildModelMatrix(glm::vec3(25.0f, 0.02f, -5.0f), glm::vec3(-90.0f, -30.0f, 0.0f), glm::vec3(0.25f)));
		iglu->Draw(*activeShader);

		activeShader->setMat4("model", BuildModelMatrix(glm::vec3(15.0f, 13.0f, 15.0f), bearRotation + glm::vec3(-90.0f, -30.0f, 0.0f), glm::vec3(1.8f)));
		bear->Draw(*activeShader);

		activeShader->setMat4("model", BuildModelMatrix(glm::vec3(10.0f, 4.0f + pinguinoDeathOffsetY, -15.0f), pinguinoRotation + glm::vec3(-90.0f, 0.0f, 180.0f), glm::vec3(1.0f)));
		pinguino->Draw(*activeShader);

		{
			float balanceo = sin((float)glfwGetTime() * 1.5f) * 0.5f;
			float inclinacion = sin((float)glfwGetTime() * 0.8f) * 5.0f;
			activeShader->setMat4("model", BuildModelMatrix(
				glm::vec3(10.0f, -8.0f + balanceo, -65.0f),
				orcaRotation + glm::vec3(-90.0f + inclinacion, 0.0f, 0.0f),
				glm::vec3(4.5f)));
			orca->Draw(*activeShader);
		}

		activeShader->setMat4("model", BuildModelMatrix(glm::vec3(10.0f, 2.5f, -125.0f), wolfRotation + glm::vec3(-90.0f, 0.0f, 0.0f), glm::vec3(5.5f)));
		wolf->Draw(*activeShader);
		activeShader->setMat4("model", BuildModelMatrix(glm::vec3(15.0f, 2.5f, -128.0f), wolfRotation + glm::vec3(-90.0f, 0.0f, 3.0f), glm::vec3(5.2f)));
		wolf->Draw(*activeShader);
		activeShader->setMat4("model", BuildModelMatrix(glm::vec3(20.50f, 2.5f, -128.0f), wolfRotation + glm::vec3(-90.0f, 0.0f, -1.0f), glm::vec3(5.4f)));
		wolf->Draw(*activeShader);

		activeShader->setMat4("model", BuildModelMatrix(glm::vec3(-50.0f, 4.95f, -130.0f), leonMarinoRotation + glm::vec3(-90.0f, 0.0f, 0.0f), glm::vec3(0.7f)));
		leonMarino->Draw(*activeShader);
		activeShader->setMat4("model", BuildModelMatrix(glm::vec3(-10.0f, 5.05f, -130.0f), leonMarinoRotation + glm::vec3(-90.0f, 0.0f, 90.0f), glm::vec3(1.65f)));
		leonMarino->Draw(*activeShader);

		activeShader->setMat4("model", BuildModelMatrix(
			glm::vec3(-22.0f, 5.1f, 12.0f),
			zorroRotation + glm::vec3(-90.0f, 0.0f, 90.0f),
			glm::vec3(1.0f) * zorroBreathingScaleFactor));
		zorro->Draw(*activeShader);

		activeShader->setMat4("model", BuildModelMatrix(glm::vec3(48.0f, 6.8f, -100.0f), renoRotation + glm::vec3(-90.0f, 0.0f, -150.0f), glm::vec3(1.8f)));
		reno->Draw(*activeShader);

		if (sceneMode < 2) {
			activeShader->setMat4("model", BuildModelMatrix(glm::vec3(0.0f, 22.0f, 17.5f), osoARotation + glm::vec3(90, 180, 90), glm::vec3(1.2f)));
			osoA->Draw(*activeShader);
		}

		if (sceneMode == 0) {
			activeShader->setMat4("model", BuildModelMatrix(glm::vec3(5.5f, 5.0f + pinguinoDeathOffsetY, -10.0f), pinguinoRotation + glm::vec3(-90.0f, 0.0f, 160.0f), glm::vec3(1.15f)));
			pinguino->Draw(*activeShader);
		}

		if (sceneMode == 1) {
			activeShader->setMat4("model", tanqueGrandeModel);
			tanqueGrande->Draw(*activeShader);
		}

		// --- CONTAMINACIÓN NIVEL 1 (Barriles y Gas) ---
		if (sceneMode >= 1) {
			// Aparecen unos tanques de gas cerca del clúster
			activeShader->setMat4("model", BuildModelMatrix(glm::vec3(-90.0f, 0.0f, 5.0f), gasRotation + glm::vec3(-90.0f, 0.0f, 0.0f), glm::vec3(1.0f)));
			gas->Draw(*activeShader);

			activeShader->setMat4("model", BuildModelMatrix(glm::vec3(-90.0f, 0.0f, 6.0f), tanquesRotation + glm::vec3(-90.0f, 0.0f, 0.0f), glm::vec3(1.2f)));
			tanques->Draw(*activeShader);

		}

		// --- CONTAMINACIÓN NIVEL 2 (Plataforma Petrolera y Desastre) ---
		if (sceneMode == 2) {
			// La gran plataforma petrolera en el centro del mar
			activeShader->setMat4("model", BuildModelMatrix(glm::vec3(90.0f, 60.0f, -30.0f), rigRotation + glm::vec3(-90.0f, 0.0f, 0.0f)	, glm::vec3(1.0f)));
			rig->Draw(*activeShader);

			// El barco hundiéndose (Titanic o Ship)
			activeShader->setMat4("model", BuildModelMatrix(glm::vec3(0.0f, 3.0f, -200.0f), shipRotation + glm::vec3(90, 90, 10), glm::vec3(0.1f)));
			ship->Draw(*activeShader);

			// Derrame de petróleo
			activeShader->setMat4("model", BuildModelMatrix(glm::vec3(69.0f, 19.0f, -12.0f), tanqueDerramadoRotation + glm::vec3(-90.0f, 0.0f, 0.0f), glm::vec3(2.5f)));
			tanqueDerramado->Draw(*activeShader);
		}

		if (sceneMode >= 1) {
			// Carguero
			activeShader->setMat4("model", cargoModel);
			cargo->Draw(*activeShader);
		}

		if (sceneMode == 0) {
			activeShader->setMat4("model", barconewModel);
			barconew->Draw(*activeShader);

			activeShader->setMat4("model", pexDoradoModel);
			pexDorado->Draw(*activeShader);

			pez->UpdateAnimation(deltaTime);
			dynamicShader->use();
			dynamicShader->setMat4("projection", projection);
			dynamicShader->setMat4("view", view);
			dynamicShader->setMat4("model", pezModel);
			dynamicShader->setMat4("gBones", MAX_RIGGING_BONES, pez->gBones);
			pez->Draw(*dynamicShader);
			// restore active shader
			activeShader->use();
		}

		// (Iceberg grande final ya gestionado en DrawSceneIcebergs)

		// =========================================================
		// CONFIGURACIÓN DE LUCES: DE NÍTIDO A GRIS INDUSTRIAL
		// =========================================================

		if (sceneMode == 0) {
			gLights[0].Color = glm::vec4(0.2f, 0.2f, 0.2f, 1.0f);
			gLights[1].Color = glm::vec4(0.2f, 0.2f, 0.2f, 1.0f);
			gLights[2].Color = glm::vec4(0.15f, 0.15f, 0.15f, 1.0f);
			gLights[3].Color = glm::vec4(0.15f, 0.15f, 0.15f, 1.0f);
			for (int i = 0; i < 4; i++) gLights[i].Power = glm::vec4(60.0f, 60.0f, 60.0f, 1.0f);
		}
		else if (sceneMode == 1) {
			// MODO 1: Contaminación inicial (Grisáceo sutil)
			// Usamos un gris neutro con un toque mínimo de amarillo sucio
			glm::vec4 grayDust = glm::vec4(0.25f, 0.25f, 0.22f, 1.0f);
			for (int i = 0; i < 4; i++) {
				gLights[i].Color = grayDust;
				gLights[i].Power = glm::vec4(45.0f, 45.0f, 45.0f, 1.0f); // Bajamos brillo
			}
		}
		else if (sceneMode == 2) {
			// MODO 2: Desastre (Gris profundo y opaco)
			// Colores muy bajos para que el blanco del hielo desaparezca
			glm::vec4 industrialGray = glm::vec4(0.12f, 0.12f, 0.13f, 1.0f);
			for (int i = 0; i < 4; i++) {
				gLights[i].Color = industrialGray;
				// Luz muy débil para dar sensación de día nublado por contaminación
				gLights[i].Power = glm::vec4(25.0f, 25.0f, 25.0f, 1.0f);
			}
		}

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

			DrawSceneIcebergsFresnel(*fresnelShader, derretimiento);

			glDepthFunc(GL_LESS);
		}

		if (!activeCamera) {
			personaje->UpdateAnimation(deltaTime);
			dynamicShader->use();
			dynamicShader->setMat4("projection", projection);
			dynamicShader->setMat4("view", view);
			dynamicShader->setMat4("model", personajePlayerModel);
			dynamicShader->setMat4("gBones", MAX_RIGGING_BONES, personaje->gBones);
			personaje->Draw(*dynamicShader);
			activeShader->use();
		}

		activeShader->setMat4("model", sealModel);
		seal->Draw(*activeShader);

		trabajadorAnimado->UpdateAnimation(deltaTime);
			dynamicShader->use();
			dynamicShader->setMat4("projection", projection);
			dynamicShader->setMat4("view", view);
			dynamicShader->setMat4("model", trabajadorAnimadoModel);
			dynamicShader->setMat4("gBones", MAX_RIGGING_BONES, trabajadorAnimado->gBones);
			trabajadorAnimado->Draw(*dynamicShader);
			activeShader->use();

			trabajadoraAnimada->UpdateAnimation(deltaTime);
			dynamicShader->use();
			dynamicShader->setMat4("projection", projection);
			dynamicShader->setMat4("view", view);
			dynamicShader->setMat4("model", trabajadoraAnimadaModel);
			dynamicShader->setMat4("gBones", MAX_RIGGING_BONES, trabajadoraAnimada->gBones);
			trabajadoraAnimada->Draw(*dynamicShader);
			activeShader->use();

		/*
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

		activeShader->setMat4("model", shipModel);
		ship->Draw(*activeShader);

		activeShader->setMat4("model", tanqueDerramadoModel);
		tanqueDerramado->Draw(*activeShader);

		

		activeShader->setMat4("model", tanquesModel);
		tanques->Draw(*activeShader);

		activeShader->setMat4("model", titanicModel);
		titanic->Draw(*activeShader);

		activeShader->setMat4("model", wolfModel);
		wolf->Draw(*activeShader);

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
	if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
	{
		float moveStep = camera.MovementSpeed * deltaTime;
		if (activeCamera) camera.Position.y += moveStep;
		else {
			position.y += moveStep;
			camera3rd.Position = position;
			camera3rd.Position.y += trdpersonHeightOffset;
			camera3rd.Position -= trdpersonOffset * forwardView;
		}
	}
	if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
	{
		float moveStep = camera.MovementSpeed * deltaTime;
		if (activeCamera) camera.Position.y -= moveStep;
		else {
			position.y -= moveStep;
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

	static bool eightKeyPressed = false;
	if (glfwGetKey(window, GLFW_KEY_8) == GLFW_PRESS && !eightKeyPressed) {
		StartZorroBreathing();
		eightKeyPressed = true;
	}
	if (glfwGetKey(window, GLFW_KEY_8) == GLFW_RELEASE) {
		eightKeyPressed = false;
	}

	static bool nineKeyPressed = false;
	if (glfwGetKey(window, GLFW_KEY_9) == GLFW_PRESS && !nineKeyPressed) {
		pinguinoDeathTargetDown = !pinguinoDeathTargetDown;
		nineKeyPressed = true;
	}
	if (glfwGetKey(window, GLFW_KEY_9) == GLFW_RELEASE) {
		nineKeyPressed = false;
	}

	static bool zeroKeyPressed = false;
	if (glfwGetKey(window, GLFW_KEY_0) == GLFW_PRESS && !zeroKeyPressed) {
		StartWolfCelebration();
		zeroKeyPressed = true;
	}
	if (glfwGetKey(window, GLFW_KEY_0) == GLFW_RELEASE) {
		zeroKeyPressed = false;
	}

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
	// F5: reiniciar progresión dinámica de contaminación
	static bool f5KeyPressed = false;
	if (glfwGetKey(window, GLFW_KEY_F5) == GLFW_PRESS && !f5KeyPressed) {
		ResetPollutionGameplay();
		f5KeyPressed = true;
	}
	if (glfwGetKey(window, GLFW_KEY_F5) == GLFW_RELEASE) {
		f5KeyPressed = false;
	}

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

	// Toggle animación de la ola con la tecla P
	static bool pKeyPressed = false;
	if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS && !pKeyPressed) {
		waveAnimationActive = !waveAnimationActive;
		pKeyPressed = true;
		const char* waveState = waveAnimationActive ? "activa" : "estática";
		std::cout << "Ola ahora: " << waveState << std::endl;
	}
	if (glfwGetKey(window, GLFW_KEY_P) == GLFW_RELEASE) {
		pKeyPressed = false;
	}
	
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
