#pragma once
#include "object.h"
GLFWwindow* StartGLU();
GLuint CreateShaderProgram(const char* vs, const char* fs);
void CreateUnitSphere(int sec=50, int stk=50);
void CreateStarfield();
void InitSpacetimeGrid();
void UpdateSpacetimeGrid(const vector<Object>& objects);
void printHelp();
