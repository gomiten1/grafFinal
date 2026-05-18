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
void InitIceDiscMesh();
void DrawIceDisc(Shader& shader, int lightingMode, const glm::mat4& projection, const glm::mat4& view, float derretimiento);
void DrawIceDiscRingLargeIcebergs(Shader& shader, float derretimiento, int& slot);
void DrawIceDiscScatteredHummocks(Shader& shader, float derretimiento, int& slot);
void DrawCenterIglus(Shader& shader, float derretimiento);
void DrawCenterLandWildlife(Shader& shader, float derretimiento);
void DrawOpenWaterSwimmers(Shader& shader, Shader& skinShader, const glm::mat4& projection, const glm::mat4& view, float deltaTime);

// Definición de callbacks
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);

// Gobals
GLFWwindow* window;
// 0 = Saludable, 1 = Contaminación inicial, 2 = Desastre industrial
int sceneMode = 0;
bool activeCamera = false; // false = tercera persona; F2 = primera persona

Material material01;

// --- Sistema dinámico de contaminación (extractores + derretimiento de icebergs) ---
const float kOilPumpSpawnInterval = 10.0f;
const float kOilPumpFirstSpawnDelay = 10.0f;
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
std::vector<float> icebergSlotDistances;
static double sOilPumpClockOrigin = -1.0;
static double sOilPumpLastSpawnWall = -1.0;
float icebergMeltTimer = 0.0f;
int oilPumpSpawnCursor = 0;
bool pollutionGameplayInitialized = false;
static float gSkyPollutionApplied = -1.0f;

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

// Disco de hielo plano (radio = kIceDiscDiameterFactor * olaScale.x; 6.0 ≈ 10× el tamaño previo de 0.6)
static const float kIceDiscDiameterFactor = 4.0f; // 10× el diámetro anterior (0.6 → 6.0)
static const float kIceDiscThickness = 0.45f;
static const float kIceDiscSurfaceY = 1.85f; // por encima del oleaje
static const int kClassicIcebergSlotCount = 36;
static const int kDiscRingLargeSlotCount = 12;
static const int kScatteredHummockSlotCount = 20;
static const int kTotalIcebergSlots = kClassicIcebergSlotCount + kDiscRingLargeSlotCount + kScatteredHummockSlotCount;
static float gIceDiscLayoutRadius = 30.0f; // fijo para icebergs/animales (no se encoge con contaminación)

static unsigned int iceDiscVAO = 0;
static unsigned int iceDiscVBO = 0;
static unsigned int iceDiscEBO = 0;
static GLsizei iceDiscIndexCount = 0;
static unsigned int iceDiscWhiteTex = 0;
static Material iceDiscSavedMaterial;

static float GetPollutionBlend();
static glm::vec3 IceDiscSurfacePos(float angleDeg, float radiusFrac, float yOnIce = 0.0f);
static float WildlifeDistXZ(const glm::vec3& pos);
static bool IsWildlifeVisibleAtPos(const glm::vec3& pos);
static float WildlifeExtraSinkAtPos(const glm::vec3& pos);
static void RecordIcebergSlotDistance(int slotIndex, const glm::vec3& worldPos);
static void SyncIcebergSlotsToPollution();
static void UpdateEnvironmentForPollution();
static void UpdateIceDiscLayoutRadius();
static float GetIceDiscDrawScale();

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
	// 36 clásicos + 12 grandes en el borde del disco + 20 mini en el disco
	icebergSlotVisible.assign(kTotalIcebergSlots, true);
	icebergSlotDistances.assign(kTotalIcebergSlots, 0.0f);
	activeOilPumps.clear();
	sOilPumpClockOrigin = -1.0;
	sOilPumpLastSpawnWall = -1.0;
	icebergMeltTimer = 0.0f;
	oilPumpSpawnCursor = 0;
	sceneMode = 0;
	gSkyPollutionApplied = -1.0f;
	pollutionGameplayInitialized = true;
}

void ResetPollutionGameplay() {
	InitPollutionGameplay();
	UpdateEnvironmentForPollution();
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
		std::fill(icebergSlotVisible.begin(), icebergSlotVisible.end(), true);
	}
}

void UpdatePollutionGameplay() {
	if (!pollutionGameplayInitialized) {
		InitPollutionGameplay();
	}

	RemoveOilPumpsPlayerTouch();

	// Reloj de pared desde el primer frame jugable (no acumula el tiempo de carga de modelos)
	if (sOilPumpClockOrigin < 0.0)
		sOilPumpClockOrigin = glfwGetTime();

	const double now = glfwGetTime();
	const float sessionElapsed = (float)(now - sOilPumpClockOrigin);

	// En primera persona (F2) no se agregan extractores automáticamente
	if (sceneMode < 2 && !activeCamera && sessionElapsed >= kOilPumpFirstSpawnDelay) {
		if (activeOilPumps.empty()) {
			SpawnOilPump();
			sOilPumpLastSpawnWall = now;
		}
		else if (sOilPumpLastSpawnWall < 0.0 ||
			(now - sOilPumpLastSpawnWall) >= (double)kOilPumpSpawnInterval) {
			SpawnOilPump();
			sOilPumpLastSpawnWall = now;
		}
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
	auto drawSlot = [&](const glm::vec3& worldPos, const glm::mat4& modelMat, Model* mesh) {
		RecordIcebergSlotDistance(slot, worldPos);
		if (slot < (int)icebergSlotVisible.size() && icebergSlotVisible[slot]) {
			shader.setMat4("model", modelMat);
			mesh->Draw(shader);
		}
		slot++;
	};

	// Layout clásico (commit d8587bd)
	drawSlot(icebergChicoPosition, icebergChicoModel, icebergChico);
	drawSlot(icebergGrandePosition, icebergGrandeModel, icebergGrande);

	drawSlot(icebergDPosition + glm::vec3(-2.0f, derretimiento, 15.0f),
		BuildModelMatrix(icebergDPosition + glm::vec3(-2.0f, derretimiento, 15.0f), icebergDRotation, glm::vec3(2.5f)), icebergD);
	drawSlot(icebergDPosition + glm::vec3(2.0f, derretimiento * -0.1f, 15.5f),
		BuildModelMatrix(icebergDPosition + glm::vec3(2.0f, derretimiento * -0.1f, 15.5f), icebergDRotation + glm::vec3(0.0f, 15.0f, 0.0f), glm::vec3(2.2f)), icebergD);
	drawSlot(icebergDPosition + glm::vec3(-2.5f, derretimiento * -0.05f, 19.0f),
		BuildModelMatrix(icebergDPosition + glm::vec3(-2.5f, derretimiento * -0.05f, 19.0f), icebergDRotation + glm::vec3(0.0f, -10.0f, 0.0f), glm::vec3(2.8f)), icebergD);
	drawSlot(icebergDPosition + glm::vec3(2.5f, derretimiento * 0.0f, 19.5f),
		BuildModelMatrix(icebergDPosition + glm::vec3(2.5f, derretimiento * 0.0f, 19.5f), icebergDRotation + glm::vec3(0.0f, 45.0f, 0.0f), glm::vec3(2.3f)), icebergD);

	drawSlot(glm::vec3(0.0f, derretimiento * 0.2f, 17.5f),
		BuildModelMatrix(glm::vec3(0.0f, derretimiento * 0.2f, 17.5f), icebergARotation, glm::vec3(3.5f)), icebergA);

	const glm::vec3 miniScale(1.5f);
	drawSlot(icebergDPosition + glm::vec3(8.0f, derretimiento * 0.0f, 15.0f),
		BuildModelMatrix(icebergDPosition + glm::vec3(8.0f, derretimiento * 0.0f, 15.0f), icebergDRotation + glm::vec3(0.0f, 10.0f, 0.0f), miniScale), icebergD);
	drawSlot(icebergDPosition + glm::vec3(12.0f, derretimiento * -0.05f, 15.0f),
		BuildModelMatrix(icebergDPosition + glm::vec3(12.0f, derretimiento * -0.05f, 15.0f), icebergDRotation + glm::vec3(0.0f, 45.0f, 0.0f), miniScale), icebergD);
	drawSlot(icebergDPosition + glm::vec3(8.5f, derretimiento * 0.0f, 18.0f),
		BuildModelMatrix(icebergDPosition + glm::vec3(8.5f, derretimiento * 0.0f, 18.0f), icebergDRotation + glm::vec3(0.0f, -20.0f, 0.0f), miniScale), icebergD);
	drawSlot(icebergDPosition + glm::vec3(12.5f, derretimiento * -0.05f, 18.0f),
		BuildModelMatrix(icebergDPosition + glm::vec3(12.5f, derretimiento * -0.05f, 18.0f), icebergDRotation + glm::vec3(0.0f, 75.0f, 0.0f), miniScale), icebergD);
	drawSlot(icebergDPosition + glm::vec3(8.0f, derretimiento * -0.1f, 21.0f),
		BuildModelMatrix(icebergDPosition + glm::vec3(8.0f, derretimiento * -0.1f, 21.0f), icebergDRotation + glm::vec3(0.0f, 130.0f, 0.0f), miniScale), icebergD);
	drawSlot(icebergDPosition + glm::vec3(12.0f, derretimiento * 0.0f, 21.0f),
		BuildModelMatrix(icebergDPosition + glm::vec3(12.0f, derretimiento * 0.0f, 21.0f), icebergDRotation + glm::vec3(0.0f, -5.0f, 0.0f), miniScale), icebergD);

	drawSlot(icebergDPosition + glm::vec3(-8.0f, derretimiento * -0.05f, 35.0f),
		BuildModelMatrix(icebergDPosition + glm::vec3(-8.0f, derretimiento * -0.05f, 35.0f), icebergDRotation + glm::vec3(0.0f, -25.0f, 0.0f), glm::vec3(1.2f)), icebergD);
	drawSlot(icebergDPosition + glm::vec3(-11.0f, derretimiento * -0.02f, 36.0f),
		BuildModelMatrix(icebergDPosition + glm::vec3(-11.0f, derretimiento * -0.02f, 36.0f), icebergDRotation, glm::vec3(0.6f)), icebergD);
	drawSlot(icebergDPosition + glm::vec3(-15.0f, derretimiento * 0.0f, 40.0f),
		BuildModelMatrix(icebergDPosition + glm::vec3(-15.0f, derretimiento * 0.0f, 40.0f), icebergDRotation, glm::vec3(0.5f)), icebergD);
	drawSlot(glm::vec3(-18.0f, derretimiento * -0.05f, 42.0f),
		BuildModelMatrix(glm::vec3(-18.0f, derretimiento * -0.05f, 42.0f), icebergDRotation, glm::vec3(0.7f)), icebergD);
	drawSlot(glm::vec3(-12.0f, derretimiento * 0.0f, 44.0f),
		BuildModelMatrix(glm::vec3(-12.0f, derretimiento * 0.0f, 44.0f), icebergDRotation, glm::vec3(0.4f)), icebergD);

	drawSlot(glm::vec3(-20.0f, derretimiento * -0.2f, -15.0f),
		BuildModelMatrix(glm::vec3(-20.0f, derretimiento * -0.2f, -15.0f), icebergDRotation + glm::vec3(10.0f, 0.0f, 0.0f), glm::vec3(1.0f)), icebergD);
	drawSlot(glm::vec3(0.0f, derretimiento * -0.1f, -25.0f),
		BuildModelMatrix(glm::vec3(0.0f, derretimiento * -0.1f, -25.0f), icebergDRotation + glm::vec3(-10.0f, 0.0f, 0.0f), glm::vec3(1.5f)), icebergD);
	drawSlot(glm::vec3(20.0f, derretimiento * -0.2f, -18.0f),
		BuildModelMatrix(glm::vec3(20.0f, derretimiento * -0.2f, -18.0f), icebergDRotation + glm::vec3(-9.0f, 0.0f, 0.0f), glm::vec3(1.2f)), icebergD);
	drawSlot(glm::vec3(5.0f, derretimiento * -0.1f, -10.0f),
		BuildModelMatrix(glm::vec3(5.0f, derretimiento * -0.1f, -10.0f), icebergDRotation + glm::vec3(-7.0f, 0.0f, 0.0f), glm::vec3(1.5f)), icebergD);

	drawSlot(glm::vec3(-85.0f, derretimiento * -4.0f, -100.0f),
		BuildModelMatrix(glm::vec3(-85.0f, derretimiento * -4.0f, -100.0f), glm::vec3(0.0f, 180.0f, 0.0f), glm::vec3(18.0f)), icebergGrande);
	drawSlot(glm::vec3(90.0f, derretimiento * -3.5f, -85.0f),
		BuildModelMatrix(glm::vec3(90.0f, derretimiento * -3.5f, -85.0f), glm::vec3(0.0f, 45.0f, 0.0f), glm::vec3(12.0f)), icebergChico);

	drawSlot(glm::vec3(-50.0f, derretimiento * -0.2f, -120.0f),
		BuildModelMatrix(glm::vec3(-50.0f, derretimiento * -0.2f, -120.0f), icebergDRotation + glm::vec3(10.0f, 0.0f, 0.0f), glm::vec3(2.5f)), icebergD);
	drawSlot(glm::vec3(-10.0f, derretimiento * -0.1f, -140.0f),
		BuildModelMatrix(glm::vec3(-10.0f, derretimiento * -0.1f, -140.0f), icebergDRotation + glm::vec3(-10.0f, 0.0f, 0.0f), glm::vec3(3.0f)), icebergD);
	drawSlot(glm::vec3(40.0f, derretimiento * -0.2f, -115.0f),
		BuildModelMatrix(glm::vec3(40.0f, derretimiento * -0.2f, -115.0f), icebergDRotation + glm::vec3(-9.0f, 0.0f, 0.0f), glm::vec3(2.8f)), icebergD);
	drawSlot(glm::vec3(15.0f, derretimiento * -0.1f, -130.0f),
		BuildModelMatrix(glm::vec3(15.0f, derretimiento * -0.1f, -130.0f), icebergDRotation + glm::vec3(-7.0f, 0.0f, 0.0f), glm::vec3(2.0f)), icebergD);

	drawSlot(glm::vec3(-15.0f, derretimiento * 0.1f, 20.0f),
		BuildModelMatrix(glm::vec3(-15.0f, derretimiento * 0.1f, 20.0f), icebergARotation + glm::vec3(0.0f, 45.0f, 0.0f), glm::vec3(3.5f)), icebergA);
	drawSlot(glm::vec3(18.0f, derretimiento * 0.0f, 16.0f),
		BuildModelMatrix(glm::vec3(18.0f, derretimiento * 0.0f, 16.0f), icebergARotation + glm::vec3(0.0f, -20.0f, 0.0f), glm::vec3(2.0f)), icebergA);
	drawSlot(glm::vec3(5.0f, derretimiento * -0.5f, 45.0f),
		BuildModelMatrix(glm::vec3(5.0f, derretimiento * -0.5f, 45.0f), icebergARotation + glm::vec3(0.0f, 90.0f, 0.0f), glm::vec3(4.0f)), icebergA);
	drawSlot(glm::vec3(-40.0f, derretimiento * -0.2f, -90.0f),
		BuildModelMatrix(glm::vec3(-40.0f, derretimiento * -0.2f, -90.0f), icebergARotation, glm::vec3(6.0f)), icebergA);
	drawSlot(glm::vec3(45.0f, derretimiento * -0.2f, -110.0f),
		BuildModelMatrix(glm::vec3(45.0f, derretimiento * -0.2f, -110.0f), icebergARotation + glm::vec3(0.0f, 180.0f, 0.0f), glm::vec3(5.5f)), icebergA);

	drawSlot(glm::vec3(-20.0f, derretimiento * 0.0f, 10.0f),
		BuildModelMatrix(glm::vec3(-20.0f, derretimiento * 0.0f, 10.0f), icebergDRotation, glm::vec3(2.0f)), icebergD);
	drawSlot(glm::vec3(25.0f, derretimiento * -0.1f, -5.0f),
		BuildModelMatrix(glm::vec3(25.0f, derretimiento * -0.1f, -5.0f), icebergDRotation, glm::vec3(1.5f)), icebergD);

	RecordIcebergSlotDistance(slot, icebergGrandePosition);
	if (slot < (int)icebergSlotVisible.size() && icebergSlotVisible[slot]) {
		if (sceneMode < 2) {
			shader.setMat4("model", icebergGrandeModel);
			icebergGrande->Draw(shader);
		}
		else {
			const glm::vec3 meltPos = icebergGrandePosition + glm::vec3(0.0f, -5.0f, 0.0f);
			shader.setMat4("model", BuildModelMatrix(meltPos, icebergGrandeRotation, icebergGrandeScale * 0.5f));
			icebergGrande->Draw(shader);
		}
	}
	slot++;

	// Grandes alrededor del diámetro del disco blanco
	DrawIceDiscRingLargeIcebergs(shader, derretimiento, slot);

	// Mini icebergs en el disco blanco (contaminación: se ocultan de afuera hacia adentro)
	DrawIceDiscScatteredHummocks(shader, derretimiento, slot);
	SyncIcebergSlotsToPollution();
}

void DrawIceDiscRingLargeIcebergs(Shader& shader, float derretimiento, int& slot) {
	UpdateIceDiscLayoutRadius();
	const float yBase = kIceDiscSurfaceY + (derretimiento - 1.0f) * -0.08f;

	struct RingSpec {
		float angleDeg;
		float radiusMul;
		float scale;
		float yawExtra;
	};

	// Anillo justo fuera del borde del disco (radio layout ≈ diámetro blanco a escala 1)
	static const RingSpec ring[] = {
		{ 0.0f,   1.08f, 21.0f,  0.0f },
		{ 30.0f,  1.10f, 23.0f,  6.0f },
		{ 60.0f,  1.09f, 19.5f, -4.0f },
		{ 90.0f,  1.11f, 22.5f,  10.0f },
		{ 120.0f, 1.08f, 20.0f,  -8.0f },
		{ 150.0f, 1.10f, 24.0f,  4.0f },
		{ 180.0f, 1.09f, 21.5f,  0.0f },
		{ 210.0f, 1.11f, 23.5f,  -6.0f },
		{ 240.0f, 1.08f, 20.5f,  12.0f },
		{ 270.0f, 1.10f, 22.0f,  -2.0f },
		{ 300.0f, 1.09f, 19.0f,  8.0f },
		{ 330.0f, 1.10f, 21.0f,  -10.0f },
	};
	const int count = (int)(sizeof(ring) / sizeof(ring[0]));

	for (int i = 0; i < count; ++i) {
		const RingSpec& spec = ring[i];
		const float rad = glm::radians(spec.angleDeg);
		const float radius = gIceDiscLayoutRadius * spec.radiusMul;
		const glm::vec3 pos(radius * cosf(rad), yBase, radius * sinf(rad));
		const float faceYaw = spec.angleDeg + 180.0f + spec.yawExtra;
		const glm::vec3 scale(spec.scale);

		RecordIcebergSlotDistance(slot, pos);
		if (slot < (int)icebergSlotVisible.size() && icebergSlotVisible[slot]) {
			shader.setMat4("model", BuildModelMatrix(
				pos, glm::vec3(0.0f, faceYaw, 0.0f), scale));
			icebergGrande->Draw(shader);
		}
		slot++;
	}
}

void DrawIceDiscScatteredHummocks(Shader& shader, float derretimiento, int& slot) {
	UpdateIceDiscLayoutRadius();
	const float yIce = kIceDiscSurfaceY + (derretimiento - 1.0f) * -0.08f;

	struct HummockSpec {
		float angleDeg;
		float radiusFrac;
		int kind; // 0 = A (montaña), 1 = D (placa)
		float scale;
		float yawExtra;
	};

	static const HummockSpec hummocks[] = {
		{ 5.0f,   0.30f, 0, 2.6f,  0.0f },
		{ 22.0f,  0.34f, 1, 1.5f,  18.0f },
		{ 41.0f,  0.38f, 0, 3.0f,  -12.0f },
		{ 58.0f,  0.32f, 1, 1.7f,  42.0f },
		{ 76.0f,  0.42f, 0, 2.8f,  8.0f },
		{ 94.0f,  0.30f, 1, 1.4f,  -25.0f },
		{ 112.0f, 0.35f, 0, 3.2f,  55.0f },
		{ 131.0f, 0.32f, 1, 1.6f,  10.0f },
		{ 149.0f, 0.42f, 0, 2.5f,  -8.0f },
		{ 158.0f, 0.50f, 1, 2.2f,  10.0f },
		{ 186.0f, 0.36f, 0, 2.9f,  22.0f },
		{ 204.0f, 0.28f, 1, 1.3f,  -15.0f },
		{ 223.0f, 0.44f, 0, 3.1f,  35.0f },
		{ 241.0f, 0.33f, 1, 1.5f,  5.0f },
		{ 259.0f, 0.33f, 0, 2.7f,  -20.0f },
		{ 278.0f, 0.40f, 1, 1.9f,  88.0f },
		{ 296.0f, 0.31f, 0, 2.4f,  12.0f },
		{ 315.0f, 0.37f, 1, 1.7f,  -35.0f },
		{ 332.0f, 0.46f, 1, 2.0f,  48.0f },
		{ 352.0f, 0.29f, 1, 1.4f,  62.0f },
	};
	const int count = (int)(sizeof(hummocks) / sizeof(hummocks[0]));

	for (int i = 0; i < count; ++i) {
		const HummockSpec& spec = hummocks[i];
		const float rad = glm::radians(spec.angleDeg);
		const float radius = gIceDiscLayoutRadius * spec.radiusFrac;
		const glm::vec3 pos(radius * cosf(rad), yIce, radius * sinf(rad));
		const float faceYaw = spec.angleDeg + 180.0f + spec.yawExtra;

		RecordIcebergSlotDistance(slot, pos);
		if (slot < (int)icebergSlotVisible.size() && icebergSlotVisible[slot]) {
			if (spec.kind == 0) {
				shader.setMat4("model", BuildModelMatrix(pos, icebergARotation + glm::vec3(0.0f, faceYaw, 0.0f), glm::vec3(spec.scale)));
				icebergA->Draw(shader);
			}
			else {
				shader.setMat4("model", BuildModelMatrix(pos, icebergDRotation + glm::vec3(0.0f, faceYaw, 0.0f), glm::vec3(spec.scale)));
				icebergD->Draw(shader);
			}
		}
		slot++;
	}
}

void DrawCenterIglus(Shader& shader, float derretimiento) {
	(void)derretimiento;
	shader.setMat4("model", BuildModelMatrix(
		glm::vec3(-20.0f, 0.05f, 10.0f), glm::vec3(-90.0f, 45.0f, 0.0f), glm::vec3(0.3f)));
	iglu->Draw(shader);
	shader.setMat4("model", BuildModelMatrix(
		glm::vec3(25.0f, 0.02f, -5.0f), glm::vec3(-90.0f, -30.0f, 0.0f), glm::vec3(0.25f)));
	iglu->Draw(shader);
}

void DrawCenterLandWildlife(Shader& shader, float derretimiento) {
	(void)derretimiento;

	static const float kOnDiscY = 1.15f;
	static const float kPinguinoOnDiscExtraY = 0.55f;
	static const float kRenoOnDiscExtraY = 0.70f;
	static const float kWolfOnDiscExtraY = 0.45f;

	auto drawAt = [&](const glm::vec3& basePos, auto drawFn) {
		if (!IsWildlifeVisibleAtPos(basePos)) {
			return;
		}
		const float sink = WildlifeExtraSinkAtPos(basePos);
		drawFn(sink);
	};

	auto drawPinguino = [&](float ang, float rFrac, float yaw, float scale) {
		const glm::vec3 base = IceDiscSurfacePos(ang, rFrac, kOnDiscY + kPinguinoOnDiscExtraY);
		drawAt(base, [&](float sink) {
			shader.setMat4("model", BuildModelMatrix(
				glm::vec3(base.x, base.y + pinguinoDeathOffsetY + sink, base.z),
				pinguinoRotation + glm::vec3(-90.0f, 0.0f, yaw),
				glm::vec3(scale)));
			pinguino->Draw(shader);
		});
	};

	auto drawLeon = [&](float ang, float rFrac, float yaw, float scale) {
		const glm::vec3 base = IceDiscSurfacePos(ang, rFrac, kOnDiscY + 0.12f);
		drawAt(base, [&](float sink) {
			shader.setMat4("model", BuildModelMatrix(
				glm::vec3(base.x, base.y + sink, base.z),
				leonMarinoRotation + glm::vec3(-90.0f, 0.0f, yaw),
				glm::vec3(scale)));
			leonMarino->Draw(shader);
		});
	};

	auto drawWolf = [&](float ang, float rFrac, float yaw, float scale) {
		const glm::vec3 base = IceDiscSurfacePos(ang, rFrac, kOnDiscY + kWolfOnDiscExtraY);
		drawAt(base, [&](float sink) {
			shader.setMat4("model", BuildModelMatrix(
				glm::vec3(base.x, base.y + sink, base.z),
				wolfRotation + glm::vec3(-90.0f, 0.0f, yaw),
				glm::vec3(scale)));
			wolf->Draw(shader);
		});
	};

	auto drawSeal = [&](float ang, float rFrac) {
		const glm::vec3 base = IceDiscSurfacePos(ang, rFrac, kOnDiscY);
		drawAt(base, [&](float sink) {
			shader.setMat4("model", BuildModelMatrix(
				glm::vec3(base.x, base.y + sink, base.z),
				sealRotation, sealScale));
			seal->Draw(shader);
		});
	};

	// Pingüinos repartidos en el disco (15%–78% del radio)
	drawPinguino(8.0f,   0.18f, 155.0f, 1.1f);
	drawPinguino(28.0f,  0.26f, 175.0f, 1.0f);
	drawPinguino(48.0f,  0.34f, 195.0f, 0.95f);
	drawPinguino(68.0f,  0.22f, 160.0f, 1.15f);
	drawPinguino(88.0f,  0.40f, 185.0f, 1.05f);
	drawPinguino(108.0f, 0.30f, 200.0f, 1.0f);
	drawPinguino(128.0f, 0.48f, 170.0f, 0.9f);
	drawPinguino(148.0f, 0.36f, 145.0f, 1.1f);
	drawPinguino(168.0f, 0.44f, 190.0f, 0.95f);
	drawPinguino(188.0f, 0.28f, 165.0f, 1.05f);
	drawPinguino(208.0f, 0.52f, 180.0f, 1.0f);
	drawPinguino(228.0f, 0.38f, 150.0f, 1.08f);
	drawPinguino(248.0f, 0.46f, 175.0f, 0.92f);
	drawPinguino(268.0f, 0.32f, 160.0f, 1.12f);
	drawPinguino(288.0f, 0.55f, 190.0f, 0.88f);
	drawPinguino(308.0f, 0.42f, 140.0f, 1.05f);
	drawPinguino(328.0f, 0.50f, 185.0f, 0.98f);
	drawPinguino(348.0f, 0.24f, 170.0f, 1.1f);

	// Leones marinos visibles sobre el hielo blanco (escala mayor)
	drawLeon(18.0f,  0.38f, 0.0f,   1.4f);
	drawLeon(42.0f,  0.52f, 45.0f,  1.2f);
	drawLeon(75.0f,  0.45f, 90.0f,  1.65f);
	drawLeon(105.0f, 0.58f, 135.0f, 1.1f);
	drawLeon(138.0f, 0.48f, 180.0f, 1.55f);
	drawLeon(165.0f, 0.62f, 225.0f, 1.25f);
	drawLeon(198.0f, 0.42f, 270.0f, 1.7f);
	drawLeon(225.0f, 0.55f, 315.0f, 1.15f);
	drawLeon(255.0f, 0.50f, 30.0f,  1.5f);
	drawLeon(285.0f, 0.65f, 75.0f,  1.35f);
	drawLeon(315.0f, 0.44f, 120.0f, 1.6f);
	drawLeon(345.0f, 0.56f, 200.0f, 1.2f);

	// Lobos en el borde interior del disco
	drawWolf(5.0f,   0.76f, 0.0f,  4.6f);
	drawWolf(62.0f,  0.80f, 3.0f,  4.8f);
	drawWolf(118.0f, 0.74f, -2.0f, 4.5f);
	drawWolf(175.0f, 0.82f, 5.0f,  4.7f);
	drawWolf(232.0f, 0.78f, 1.0f,  4.9f);
	drawWolf(288.0f, 0.81f, -1.0f, 4.6f);

	// Focas
	drawSeal(50.0f,  0.32f);
	drawSeal(95.0f,  0.38f);
	drawSeal(155.0f, 0.35f);
	drawSeal(205.0f, 0.42f);
	drawSeal(260.0f, 0.36f);
	drawSeal(310.0f, 0.44f);

	// Zorros
	{
		const glm::vec3 base = IceDiscSurfacePos(95.0f, 0.36f, kOnDiscY);
		drawAt(base, [&](float sink) {
			shader.setMat4("model", BuildModelMatrix(
				glm::vec3(base.x, base.y + sink, base.z),
				zorroRotation + glm::vec3(-90.0f, 0.0f, 90.0f),
				glm::vec3(1.0f) * zorroBreathingScaleFactor));
			zorro->Draw(shader);
		});
	}
	{
		const glm::vec3 base = IceDiscSurfacePos(185.0f, 0.44f, kOnDiscY);
		drawAt(base, [&](float sink) {
			shader.setMat4("model", BuildModelMatrix(
				glm::vec3(base.x, base.y + sink, base.z),
				zorroRotation + glm::vec3(-90.0f, 0.0f, -40.0f),
				glm::vec3(0.95f) * zorroBreathingScaleFactor));
			zorro->Draw(shader);
		});
	}
	{
		const glm::vec3 base = IceDiscSurfacePos(268.0f, 0.40f, kOnDiscY);
		drawAt(base, [&](float sink) {
			shader.setMat4("model", BuildModelMatrix(
				glm::vec3(base.x, base.y + sink, base.z),
				zorroRotation + glm::vec3(-90.0f, 0.0f, -70.0f),
				glm::vec3(0.95f) * zorroBreathingScaleFactor));
			zorro->Draw(shader);
		});
	}
	{
		const glm::vec3 base = IceDiscSurfacePos(332.0f, 0.48f, kOnDiscY);
		drawAt(base, [&](float sink) {
			shader.setMat4("model", BuildModelMatrix(
				glm::vec3(base.x, base.y + sink, base.z),
				zorroRotation + glm::vec3(-90.0f, 0.0f, 30.0f),
				glm::vec3(0.9f) * zorroBreathingScaleFactor));
			zorro->Draw(shader);
		});
	}
	{
		const glm::vec3 base = IceDiscSurfacePos(22.0f, 0.52f, kOnDiscY);
		drawAt(base, [&](float sink) {
			shader.setMat4("model", BuildModelMatrix(
				glm::vec3(base.x, base.y + sink, base.z),
				zorroRotation + glm::vec3(-90.0f, 0.0f, 120.0f),
				glm::vec3(0.92f) * zorroBreathingScaleFactor));
			zorro->Draw(shader);
		});
	}

	// Renos
	{
		const glm::vec3 base = IceDiscSurfacePos(158.0f, 0.50f, kOnDiscY + kRenoOnDiscExtraY);
		drawAt(base, [&](float sink) {
			shader.setMat4("model", BuildModelMatrix(
				glm::vec3(base.x, base.y + sink, base.z),
				renoRotation + glm::vec3(-90.0f, 0.0f, -150.0f),
				glm::vec3(1.8f)));
			reno->Draw(shader);
		});
	}
	{
		const glm::vec3 base = IceDiscSurfacePos(298.0f, 0.46f, kOnDiscY + kRenoOnDiscExtraY);
		drawAt(base, [&](float sink) {
			shader.setMat4("model", BuildModelMatrix(
				glm::vec3(base.x, base.y + sink, base.z),
				renoRotation + glm::vec3(-90.0f, 0.0f, 30.0f),
				glm::vec3(1.7f)));
			reno->Draw(shader);
		});
	}

	// Osos pardos
	{
		const glm::vec3 base = IceDiscSurfacePos(44.0f, 0.28f, kOnDiscY + 0.35f);
		drawAt(base, [&](float sink) {
			shader.setMat4("model", BuildModelMatrix(
				glm::vec3(base.x, base.y + sink, base.z),
				bearRotation + glm::vec3(-90.0f, -30.0f, 0.0f),
				glm::vec3(1.8f)));
			bear->Draw(shader);
		});
	}
	{
		const glm::vec3 base = IceDiscSurfacePos(220.0f, 0.32f, kOnDiscY + 0.35f);
		drawAt(base, [&](float sink) {
			shader.setMat4("model", BuildModelMatrix(
				glm::vec3(base.x, base.y + sink, base.z),
				bearRotation + glm::vec3(-90.0f, 20.0f, 0.0f),
				glm::vec3(1.7f)));
			bear->Draw(shader);
		});
	}

	// Oso polar en la montaña central
	if (sceneMode < 2) {
		drawAt(glm::vec3(0.0f, 22.0f, 17.5f), [&](float sink) {
			shader.setMat4("model", BuildModelMatrix(
				glm::vec3(0.0f, 22.0f + sink, 17.5f),
				osoARotation + glm::vec3(90.0f, 180.0f, 90.0f),
				glm::vec3(1.2f)));
			osoA->Draw(shader);
		});
	}
}

static glm::vec3 OpenWaterPosition(float angleDeg, float radius, float y) {
	const float rad = glm::radians(angleDeg);
	return glm::vec3(radius * cosf(rad), y, radius * sinf(rad));
}

// Mar abierto: fuera del anillo de icebergs (~1.16R + tamaño del modelo)
static float AquaticSwimRadius() {
	UpdateIceDiscLayoutRadius();
	return gIceDiscLayoutRadius * 1.42f;
}

void DrawOpenWaterSwimmers(Shader& shader, Shader& skinShader, const glm::mat4& projection, const glm::mat4& view, float deltaTime) {
	if (GetPollutionBlend() >= 1.0f) {
		return;
	}
	UpdateIceDiscLayoutRadius();

	const float t = (float)glfwGetTime();
	const float swimR = AquaticSwimRadius();
	const float swimR2 = swimR * 0.94f;
	const float swimY = 0.12f;

	auto drawOrcaAt = [&](float angleDeg, float radius, float scale, float phase) {
		const float balanceo = sinf(t * 1.5f + phase) * 0.5f;
		const float inclinacion = sinf(t * 0.8f + phase * 0.7f) * 5.0f;
		const glm::vec3 pos = OpenWaterPosition(angleDeg, radius, -8.0f + balanceo);
		if (!IsWildlifeVisibleAtPos(pos)) {
			return;
		}
		const float sink = WildlifeExtraSinkAtPos(pos);
		shader.setMat4("model", BuildModelMatrix(
			glm::vec3(pos.x, pos.y + sink, pos.z),
			orcaRotation + glm::vec3(-90.0f + inclinacion, 0.0f, 0.0f),
			glm::vec3(scale)));
		orca->Draw(shader);
	};

	drawOrcaAt(21.0f, swimR, 4.5f, 0.0f);
	drawOrcaAt(103.0f, swimR2, 4.2f, 1.4f);
	drawOrcaAt(190.0f, swimR, 4.8f, 2.8f);
	drawOrcaAt(276.0f, swimR2, 4.4f, 4.1f);
	drawOrcaAt(347.0f, swimR, 4.6f, 5.5f);

	auto drawPezAt = [&](float angleDeg, float radius, float y, float scaleMul) {
		const glm::vec3 pos = OpenWaterPosition(angleDeg, radius, y);
		if (!IsWildlifeVisibleAtPos(pos)) {
			return;
		}
		const float sink = WildlifeExtraSinkAtPos(pos);
		const glm::vec3 drawPos(pos.x, pos.y + sink, pos.z);
		const glm::mat4 fishModelMat = BuildModelMatrix(drawPos, pezRotation, pezScale * scaleMul);
		pez->UpdateAnimation(deltaTime);
		skinShader.use();
		skinShader.setMat4("projection", projection);
		skinShader.setMat4("view", view);
		skinShader.setMat4("model", fishModelMat);
		skinShader.setMat4("gBones", MAX_RIGGING_BONES, pez->gBones);
		pez->Draw(skinShader);
		shader.use();
	};

	drawPezAt(47.0f, swimR2, swimY, 1.55f);
	drawPezAt(74.0f, swimR, swimY + 0.05f, 1.4f);
	drawPezAt(132.0f, swimR2, swimY, 1.35f);
	drawPezAt(218.0f, swimR, swimY, 1.5f);
	drawPezAt(248.0f, swimR2, swimY + 0.03f, 1.25f);
	drawPezAt(305.0f, swimR, swimY, 1.45f);

	auto drawFishAt = [&](float angleDeg, float radius, float y, float scaleMul) {
		const glm::vec3 pos = OpenWaterPosition(angleDeg, radius, y);
		if (!IsWildlifeVisibleAtPos(pos)) {
			return;
		}
		const float sink = WildlifeExtraSinkAtPos(pos);
		shader.setMat4("model", BuildModelMatrix(
			glm::vec3(pos.x, pos.y + sink, pos.z),
			fishRotation,
			fishScale * scaleMul));
		fish->Draw(shader);
	};

	drawFishAt(161.0f, swimR, swimY, 0.9f);
	drawFishAt(247.0f, swimR2, swimY, 0.85f);
	drawFishAt(327.0f, swimR, swimY + 0.02f, 0.75f);

	auto drawPexAt = [&](float angleDeg, float radius, float y, float scaleMul) {
		const glm::vec3 pos = OpenWaterPosition(angleDeg, radius, y);
		if (!IsWildlifeVisibleAtPos(pos)) {
			return;
		}
		const float sink = WildlifeExtraSinkAtPos(pos);
		shader.setMat4("model", BuildModelMatrix(
			glm::vec3(pos.x, pos.y + sink, pos.z),
			pexDoradoRotation,
			pexDoradoScale * scaleMul));
		pexDorado->Draw(shader);
	};

	drawPexAt(22.0f, swimR2, swimY + 0.12f, 1.0f);
	drawPexAt(304.0f, swimR2, swimY + 0.1f, 0.9f);
}

void DrawSceneIcebergsFresnel(Shader& shader, float derretimiento) {
	(void)derretimiento;
	if (icebergSlotVisible.size() > 0 && icebergSlotVisible[0]) {
		shader.setMat4("model", icebergChicoModel);
		icebergChico->Draw(shader);
	}
	if (icebergSlotVisible.size() > 1 && icebergSlotVisible[1]) {
		shader.setMat4("model", icebergGrandeModel);
		icebergGrande->Draw(shader);
	}
	if (icebergSlotVisible.size() > 6 && icebergSlotVisible[6]) {
		shader.setMat4("model", BuildModelMatrix(
			glm::vec3(0.0f, derretimiento * 0.2f, 17.5f), icebergARotation, glm::vec3(3.5f)));
		icebergA->Draw(shader);
	}
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
	InitIceDiscMesh();

	// Cubemap: cielo azul procedural (claro arriba, más oscuro hacia el horizonte y abajo)
	mainCubeMap = new CubeMap();
	mainCubeMap->loadProceduralSkyCubemap(128, 0.0f);

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

	lastFrame = (float)glfwGetTime();

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

static float GetPollutionBlend() {
	const int count = (int)activeOilPumps.size();
	if (count <= 0) {
		return 0.0f;
	}
	return glm::clamp((float)count / (float)kPumpsForDisaster, 0.0f, 1.0f);
}

static float WildlifeDistXZ(const glm::vec3& pos) {
	return glm::length(glm::vec2(pos.x, pos.z));
}

static glm::vec3 IceDiscSurfacePos(float angleDeg, float radiusFrac, float yOnIce) {
	UpdateIceDiscLayoutRadius();
	const float rad = glm::radians(angleDeg);
	const float r = gIceDiscLayoutRadius * radiusFrac;
	return glm::vec3(r * cosf(rad), kIceDiscSurfaceY + yOnIce, r * sinf(rad));
}

static float WildlifeMinDistOnDisc() {
	UpdateIceDiscLayoutRadius();
	return gIceDiscLayoutRadius * 0.05f;
}

static float WildlifeMaxDistOnDisc() {
	UpdateIceDiscLayoutRadius();
	return gIceDiscLayoutRadius * 0.84f;
}

static bool IsWildlifeVisibleAtPos(const glm::vec3& pos) {
	const float t = GetPollutionBlend();
	if (t >= 1.0f) {
		return false;
	}
	if (t <= 0.0f) {
		return true;
	}
	const float d = WildlifeDistXZ(pos);
	const float minD = WildlifeMinDistOnDisc();
	const float maxD = WildlifeMaxDistOnDisc();
	const float norm = glm::clamp((d - minD) / (maxD - minD + 1e-5f), 0.0f, 1.0f);
	return t < (1.0f - norm) + 0.0001f;
}

static float WildlifeExtraSinkAtPos(const glm::vec3& pos) {
	const float t = GetPollutionBlend();
	if (t <= 0.0f) {
		return 0.0f;
	}
	const float d = WildlifeDistXZ(pos);
	const float minD = WildlifeMinDistOnDisc();
	const float maxD = WildlifeMaxDistOnDisc();
	const float norm = glm::clamp((d - minD) / (maxD - minD + 1e-5f), 0.0f, 1.0f);
	const float hideAt = 1.0f - norm;
	if (t <= hideAt) {
		return 0.0f;
	}
	const float u = glm::clamp((t - hideAt) / (1.0f - hideAt + 1e-5f), 0.0f, 1.0f);
	return u * -18.0f;
}

static void RecordIcebergSlotDistance(int slotIndex, const glm::vec3& worldPos) {
	if (slotIndex >= 0 && slotIndex < (int)icebergSlotDistances.size()) {
		icebergSlotDistances[slotIndex] = WildlifeDistXZ(worldPos);
	}
}

static void SyncIcebergSlotsToPollution() {
	const int total = (int)icebergSlotVisible.size();
	if (total <= 0) {
		return;
	}
	const float t = GetPollutionBlend();
	const int hideCount = (int)floorf(t * (float)total + 0.0001f);
	std::vector<int> order(total);
	for (int i = 0; i < total; ++i) {
		order[i] = i;
	}
	std::sort(order.begin(), order.end(), [](int a, int b) {
		const float da = (a < (int)icebergSlotDistances.size()) ? icebergSlotDistances[a] : 0.0f;
		const float db = (b < (int)icebergSlotDistances.size()) ? icebergSlotDistances[b] : 0.0f;
		return da > db;
	});
	std::fill(icebergSlotVisible.begin(), icebergSlotVisible.end(), true);
	for (int i = 0; i < hideCount && i < total; ++i) {
		icebergSlotVisible[order[i]] = false;
	}
}

static void UpdateEnvironmentForPollution() {
	const float t = GetPollutionBlend();
	UpdateIceDiscLayoutRadius();
	if (mainCubeMap != nullptr && fabsf(t - gSkyPollutionApplied) > 0.007f) {
		mainCubeMap->loadProceduralSkyCubemap(128, t);
		gSkyPollutionApplied = t;
	}
}

static void UpdateIceDiscLayoutRadius() {
	gIceDiscLayoutRadius = olaScale.x * kIceDiscDiameterFactor;
}

static float GetIceDiscDrawScale() {
	return glm::mix(1.0f, 0.50f, GetPollutionBlend());
}

static Vertex MakeIceDiscVertex(const glm::vec3& pos, const glm::vec3& normal, const glm::vec2& uv) {
	Vertex v;
	v.Position = pos;
	v.Normal = normal;
	v.TexCoords = uv;
	v.Tangent = glm::vec3(1.0f, 0.0f, 0.0f);
	v.Bitangent = glm::vec3(0.0f, 1.0f, 0.0f);
	v.IDs1 = v.IDs2 = v.IDs3 = glm::vec4(0.0f);
	v.Weights1 = v.Weights2 = v.Weights3 = glm::vec4(0.0f);
	return v;
}

void InitIceDiscMesh() {
	if (iceDiscVAO != 0) {
		return;
	}
	UpdateIceDiscLayoutRadius();

	const int segments = 48;
	std::vector<Vertex> verts;
	std::vector<unsigned int> indices;
	verts.reserve(2 + (segments + 1) * 2);
	indices.reserve(segments * 12);

	const unsigned int bottomCenter = 0;
	const unsigned int topCenter = 1;
	verts.push_back(MakeIceDiscVertex(glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec2(0.5f, 0.5f)));
	verts.push_back(MakeIceDiscVertex(glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0.5f, 0.5f)));

	std::vector<unsigned int> bottomRing;
	std::vector<unsigned int> topRing;
	bottomRing.reserve(segments + 1);
	topRing.reserve(segments + 1);

	for (int i = 0; i <= segments; ++i) {
		const float t = (float)i / (float)segments;
		const float angle = t * 2.0f * glm::pi<float>();
		const float c = cosf(angle);
		const float s = sinf(angle);
		const glm::vec2 uv(t, 0.5f);

		bottomRing.push_back((unsigned int)verts.size());
		verts.push_back(MakeIceDiscVertex(glm::vec3(c, s, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), uv));

		topRing.push_back((unsigned int)verts.size());
		verts.push_back(MakeIceDiscVertex(glm::vec3(c, s, 1.0f), glm::vec3(0.0f, 0.0f, 1.0f), uv));
	}

	for (int i = 0; i < segments; ++i) {
		const unsigned int b0 = bottomRing[i];
		const unsigned int b1 = bottomRing[i + 1];
		const unsigned int t0 = topRing[i];
		const unsigned int t1 = topRing[i + 1];

		indices.push_back(bottomCenter);
		indices.push_back(b1);
		indices.push_back(b0);

		indices.push_back(topCenter);
		indices.push_back(t0);
		indices.push_back(t1);

		const float midAngle = (float)(i + 0.5f) * 2.0f * glm::pi<float>() / (float)segments;
		const glm::vec3 sideNormal(cosf(midAngle), sinf(midAngle), 0.0f);
		verts[b0].Normal = sideNormal;
		verts[b1].Normal = sideNormal;
		verts[t0].Normal = sideNormal;
		verts[t1].Normal = sideNormal;

		indices.push_back(b0);
		indices.push_back(b1);
		indices.push_back(t1);
		indices.push_back(b0);
		indices.push_back(t1);
		indices.push_back(t0);
	}
	iceDiscIndexCount = (GLsizei)indices.size();

	glGenVertexArrays(1, &iceDiscVAO);
	glGenBuffers(1, &iceDiscVBO);
	glGenBuffers(1, &iceDiscEBO);
	glBindVertexArray(iceDiscVAO);
	glBindBuffer(GL_ARRAY_BUFFER, iceDiscVBO);
	glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(Vertex), verts.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, iceDiscEBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Tangent));
	glEnableVertexAttribArray(4);
	glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Bitangent));
	glBindVertexArray(0);

	unsigned char whitePx[] = { 255, 255, 255, 255 };
	glGenTextures(1, &iceDiscWhiteTex);
	glBindTexture(GL_TEXTURE_2D, iceDiscWhiteTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, whitePx);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, 0);
}

static void ApplyMaterialToIceDiscShader(Shader& shader, int lightingMode, const Material& mat) {
	if (lightingMode == 0 || lightingMode == 2) {
		shader.setVec4("MaterialAmbientColor", mat.ambient);
		shader.setVec4("MaterialDiffuseColor", mat.diffuse);
		shader.setVec4("MaterialSpecularColor", mat.specular);
		shader.setFloat("transparency", mat.transparency);
	}
}

static Material MaterialForIceDisc(float derretimiento) {
	Material mat;
	mat.transparency = 1.0f;

	const float p = GetPollutionBlend();
	const glm::vec4 cleanAmbient(0.32f, 0.36f, 0.42f, 1.0f);
	const glm::vec4 cleanDiffuse(0.94f, 0.97f, 1.0f, 1.0f);
	const glm::vec4 cleanSpecular(0.55f, 0.60f, 0.68f, 1.0f);
	const glm::vec4 dirtyAmbient(0.18f, 0.19f, 0.20f, 1.0f);
	const glm::vec4 dirtyDiffuse(0.42f, 0.44f, 0.46f, 1.0f);
	const glm::vec4 dirtySpecular(0.10f, 0.10f, 0.11f, 1.0f);

	mat.ambient = glm::mix(cleanAmbient, dirtyAmbient, p);
	mat.diffuse = glm::mix(cleanDiffuse, dirtyDiffuse, p);
	mat.specular = glm::mix(cleanSpecular, dirtySpecular, p);

	if (sceneMode >= 1) {
		const float melt = glm::clamp(derretimiento * 0.06f, 0.0f, 0.35f);
		mat.ambient = glm::mix(mat.ambient, dirtyAmbient, melt);
		mat.diffuse = glm::mix(mat.diffuse, dirtyDiffuse, melt * 0.5f);
		mat.specular *= (1.0f - melt * 0.6f);
	}
	return mat;
}

void DrawIceDisc(Shader& shader, int lightingMode, const glm::mat4& projection, const glm::mat4& view, float derretimiento) {
	if (iceDiscVAO == 0) {
		return;
	}

	shader.use();
	shader.setMat4("projection", projection);
	shader.setMat4("view", view);

	iceDiscSavedMaterial = material01;
	material01 = MaterialForIceDisc(derretimiento);
	ApplyMaterialToIceDiscShader(shader, lightingMode, material01);

	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(3.0f, 3.0f);

	const float drawRadius = gIceDiscLayoutRadius * GetIceDiscDrawScale();
	const glm::mat4 discModel = BuildModelMatrix(
		glm::vec3(0.0f, kIceDiscSurfaceY, 0.0f),
		glm::vec3(-90.0f, 0.0f, 0.0f),
		glm::vec3(drawRadius, drawRadius, kIceDiscThickness));
	shader.setMat4("model", discModel);

	const GLint texLoc = glGetUniformLocation(shader.ID, "texture_diffuse1");
	if (texLoc >= 0) {
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, iceDiscWhiteTex);
		glUniform1i(texLoc, 0);
	}

	glBindVertexArray(iceDiscVAO);
	glDrawElements(GL_TRIANGLES, iceDiscIndexCount, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);

	glDisable(GL_POLYGON_OFFSET_FILL);
	material01 = iceDiscSavedMaterial;
	ApplyMaterialToIceDiscShader(shader, lightingMode, material01);
}


bool Update() {
	const float pollution = GetPollutionBlend();
	float derretimiento = 1.0f + pollution * 8.0f;

	// Cálculo del framerate (tope evita un salto enorme en el primer frame tras cargar modelos)
	float currentFrame = (float)glfwGetTime();
	deltaTime = currentFrame - lastFrame;
	if (deltaTime > 0.05f)
		deltaTime = 0.05f;
	lastFrame = currentFrame;

	// Procesa la entrada del teclado o mouse
	processInput(window);
	UpdateWolfCelebration();
	UpdatePinguinoDeath();
	UpdateZorroBreathing();
	UpdatePollutionGameplay();
	UpdateEnvironmentForPollution();

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

		// Nadadores en el mar (antes del disco blanco; desaparecen con la contaminación)
		if (pollution < 1.0f) {
			glDisable(GL_BLEND);
			glDepthMask(GL_TRUE);
			if (lightingMode == 2) {
				basicPhongShader->use();
				basicPhongShader->setMat4("projection", projection);
				basicPhongShader->setMat4("view", view);
				basicPhongShader->setVec4("LightColor", gSimpleLight.Color);
				basicPhongShader->setVec4("LightPower", gSimpleLight.Power);
				basicPhongShader->setInt("alphaIndex", gSimpleLight.alphaIndex);
				basicPhongShader->setFloat("distance", gSimpleLight.distance);
				basicPhongShader->setVec3("lightPosition", gSimpleLight.Position);
				basicPhongShader->setVec3("lightDirection", gSimpleLight.Direction);
				basicPhongShader->setVec3("eye", eyePosition);
				DrawOpenWaterSwimmers(*basicPhongShader, *dynamicShader, projection, view, deltaTime);
			}
			else {
				mLightsShader->use();
				mLightsShader->setMat4("projection", projection);
				mLightsShader->setMat4("view", view);
				mLightsShader->setInt("numLights", (int)gLights.size());
				for (size_t li = 0; li < gLights.size(); ++li) {
					SetLightUniformVec3(mLightsShader, "Position", li, gLights[li].Position);
					SetLightUniformVec3(mLightsShader, "Direction", li, gLights[li].Direction);
					SetLightUniformVec4(mLightsShader, "Color", li, gLights[li].Color);
					SetLightUniformVec4(mLightsShader, "Power", li, gLights[li].Power);
					SetLightUniformInt(mLightsShader, "alphaIndex", li, gLights[li].alphaIndex);
					SetLightUniformFloat(mLightsShader, "distance", li, gLights[li].distance);
				}
				mLightsShader->setVec3("eye", eyePosition);
				DrawOpenWaterSwimmers(*mLightsShader, *dynamicShader, projection, view, deltaTime);
			}
		}

		// Disco de hielo blanco (encima del agua y de los nadadores del centro)
		glDisable(GL_BLEND);
		glDepthMask(GL_TRUE);
		if (lightingMode == 2) {
			basicPhongShader->use();
			basicPhongShader->setMat4("projection", projection);
			basicPhongShader->setMat4("view", view);
			basicPhongShader->setVec4("LightColor", gSimpleLight.Color);
			basicPhongShader->setVec4("LightPower", gSimpleLight.Power);
			basicPhongShader->setInt("alphaIndex", gSimpleLight.alphaIndex);
			basicPhongShader->setFloat("distance", gSimpleLight.distance);
			basicPhongShader->setVec3("lightPosition", gSimpleLight.Position);
			basicPhongShader->setVec3("lightDirection", gSimpleLight.Direction);
			basicPhongShader->setVec3("eye", eyePosition);
			DrawIceDisc(*basicPhongShader, 2, projection, view, derretimiento);
		}
		else {
			mLightsShader->use();
			mLightsShader->setMat4("projection", projection);
			mLightsShader->setMat4("view", view);
			mLightsShader->setInt("numLights", (int)gLights.size());
			for (size_t li = 0; li < gLights.size(); ++li) {
				SetLightUniformVec3(mLightsShader, "Position", li, gLights[li].Position);
				SetLightUniformVec3(mLightsShader, "Direction", li, gLights[li].Direction);
				SetLightUniformVec4(mLightsShader, "Color", li, gLights[li].Color);
				SetLightUniformVec4(mLightsShader, "Power", li, gLights[li].Power);
				SetLightUniformInt(mLightsShader, "alphaIndex", li, gLights[li].alphaIndex);
				SetLightUniformFloat(mLightsShader, "distance", li, gLights[li].distance);
			}
			mLightsShader->setVec3("eye", eyePosition);
			DrawIceDisc(*mLightsShader, 0, projection, view, derretimiento);
		}

		// restore previously active shader
		activeShader->use();
		activeShader->setMat4("projection", projection);
		activeShader->setMat4("view", view);

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

		DrawCenterIglus(*activeShader, derretimiento);
		DrawCenterLandWildlife(*activeShader, derretimiento);

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
			activeShader->setMat4("model", BuildModelMatrix(
				barconewPosition,
				barconewRotation,
				barconewScale));
			barconew->Draw(*activeShader);
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
