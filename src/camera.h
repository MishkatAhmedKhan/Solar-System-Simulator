#pragma once
#include "globals.h"
void updateCameraFront();
void scroll_callback(GLFWwindow* w, double xo, double yo);
void mouse_callback(GLFWwindow* w, double xp, double yp);
void processInput(GLFWwindow* w);
void UpdateCam(GLuint prog, glm::vec3 pos);
