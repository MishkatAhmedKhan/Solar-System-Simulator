#pragma once
#include <bits/stdc++.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cmath>
using namespace std;

// Window
inline float screenWidth = 1920.0f, screenHeight = 1080.0f;

// Scale: 1 GL unit = SCALE meters
inline const double SCALE = 1e9;

// Planet size multiplier
inline float planetSizeMultiplier = 200.0f;
inline bool trueScaleMode = false, tKeyWasPressed = false;

// Physics
inline const double Gd = 6.67430e-11;
inline const double SOFTENING = 1e6;
inline const double LIGHT_SPEED = 299792458.0;
inline const double C2 = LIGHT_SPEED * LIGHT_SPEED;

enum PhysicsMode { NEWTON, EINSTEIN };
inline PhysicsMode physicsMode = NEWTON;
inline bool gKeyWasPressed = false;

// Timing
inline float deltaTime = 0.0f, lastFrame = 0.0f;

// Camera
inline glm::vec3 cameraPos = glm::vec3(0,0,1);
inline glm::vec3 cameraFront = glm::vec3(0,0,-1);
inline glm::vec3 cameraUp = glm::vec3(0,1,0);
inline float zoomSpeed = 0, yaw_ = -90, pitch_ = 0, mouseSensitivity = 0.08f;
inline bool freeLookMode = false, fKeyWasPressed = false, firstMouse = true;
inline double lastMouseX = 0, lastMouseY = 0;

// Planet follow
inline int followPlanet = -1;
inline glm::vec3 followOffset(0.0f);
inline bool numberKeyWasPressed[9] = {};
inline bool graveKeyWasPressed = false;
inline const char* planetNames[] = {
    "Sun","Mercury","Venus","Earth","Mars","Jupiter","Saturn","Uranus","Neptune"
};

// Follow view mode: 0 = Sun-focus (camera always looks at Sun), 1 = Observer (free look from planet)
inline int followViewMode = 0;
inline bool vKeyWasPressed = false;
inline const char* followViewNames[] = { "Sun-Focus", "Observer" };

// Simulation
inline float simSpeedMultiplier = 1.0f;
inline float baseTimeScale = 1.0f; // 1 second real-time = baseTimeScale days simulation time
inline bool paused = false, pKeyWasPressed = false, oKeyWasPressed = false;

// Starfield (multi-layer)
struct StarLayer {
    GLuint VAO = 0, VBO = 0;
    int count = 0;
    float pointSize;
    float r, g, b, a;
};
inline vector<StarLayer> starLayers;

// Orbit trails
inline const int MAX_TRAIL_POINTS = 600;
inline vector<deque<glm::vec3>> orbitTrails;
inline GLuint trailVAO = 0, trailVBO = 0;
inline int trailFrameCounter = 0;
inline bool showTrails = true;

// Spacetime grid
inline GLuint spacetimeGridVAO = 0, spacetimeGridVBO = 0;
inline int spacetimeGridVertCount = 0;
inline bool showSpacetimeGrid = false;

// FPS
inline float fpsTimer = 0; inline int fpsFrames = 0; inline float currentFPS = 0;

// Sphere mesh
inline GLuint unitSphereVAO = 0, unitSphereVBO = 0;
inline int unitSphereVertexCount = 0;
