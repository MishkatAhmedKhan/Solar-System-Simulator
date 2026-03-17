#include "globals.h"
#include "shaders.h"
#include "object.h"
#include "physics.h"
#include "scene.h"
#include "camera.h"

// --- ImGui ---
#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

// --- FORCE HIGH-PERFORMANCE DEDICATED GPU (NVIDIA & AMD) ---
#ifdef _WIN32
extern "C" {
    // NVIDIA Optimus: Force dedicated GPU
    __declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001;
    // AMD PowerXpress: Force high-performance GPU
    __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif
int main() {
    GLFWwindow* window = StartGLU();
    if (!window) return -1;
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    printHelp();

    GLuint shader = CreateShaderProgram(vertexShaderSource, fragmentShaderSource);
    GLint lightLoc = glGetUniformLocation(shader, "lightPos");
    GLint ambLoc   = glGetUniformLocation(shader, "ambient");
    GLint modLoc   = glGetUniformLocation(shader, "model");
    GLint colLoc   = glGetUniformLocation(shader, "objectColor");
    GLint timeLoc  = glGetUniformLocation(shader, "uTime");
    glUseProgram(shader);

    glm::mat4 proj = glm::perspective(glm::radians(45.0f), screenWidth/screenHeight, 0.01f, 1e7f);
    glUniformMatrix4fv(glGetUniformLocation(shader,"projection"),1,GL_FALSE,glm::value_ptr(proj));
    cameraPos = glm::vec3(0, 100, 800);
    glClearColor(0,0,0,1);

    // --- SETUP IMGUI ---
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    CreateUnitSphere(200, 200);
    CreateStarfield();
    InitSpacetimeGrid();

    // Trail VBO
    glGenVertexArrays(1,&trailVAO); glGenBuffers(1,&trailVBO);
    glBindVertexArray(trailVAO); glBindBuffer(GL_ARRAY_BUFFER,trailVBO);
    glVertexAttribPointer(0,3,GL_FLOAT,0,6*sizeof(float),(void*)0); glEnableVertexAttribArray(0);
    glVertexAttribPointer(1,3,GL_FLOAT,0,6*sizeof(float),(void*)(3*sizeof(float))); glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    // Solar system objects
    vector<Object> objects {
        // Sun (static at origin initially)
        Object({0, 0, 0}, {0, 0, 0}, {0, 0, 0}, 2.0e30f, 1410, true, {1, .85f, .2f, 1}),
        // Mercury: 7.01 degree inclination
        Object({5.79e10f, 0, 0}, {0, 5.78e3f, 4.700e4f}, {0, 0, 0}, 3.285e23f, 5430, false, {.55f, .52f, .50f, 1}),
        // Venus: 3.39 degree inclination
        Object({1.082e11f, 0, 0}, {0, 2.07e3f, 3.496e4f}, {0, 0, 0}, 4.867e24f, 5243, false, {.90f, .82f, .55f, 1}),
        // Earth: 0 degree inclination (Ecliptic reference)
        Object({1.496e11f, 0, 0}, {0, 0, 2.978e4f}, {0, 0, 0}, 5.972e24f, 5515, false, {.15f, .45f, .75f, 1}),
        // Mars: 1.85 degree inclination
        Object({2.279e11f, 0, 0}, {0, 7.77e2f, 2.406e4f}, {0, 0, 0}, 6.39e23f, 3933, false, {.78f, .30f, .12f, 1}),
        // Jupiter: 1.31 degree inclination
        Object({7.785e11f, 0, 0}, {0, 2.98e2f, 1.307e4f}, {0, 0, 0}, 1.8981e27f, 1326, false, {.82f, .63f, .38f, 1}),
        // Saturn: 2.49 degree inclination
        Object({1.432e12f, 0, 0}, {0, 4.20e2f, 9.670e3f}, {0, 0, 0}, 5.683e26f, 687, false, {.87f, .78f, .50f, 1}),
        // Uranus: 0.77 degree inclination
        Object({2.871e12f, 0, 0}, {0, 91.3f, 6.799e3f}, {0, 0, 0}, 8.681e25f, 1271, false, {.55f, .82f, .87f, 1}),
        // Neptune: 1.77 degree inclination
        Object({4.495e12f, 0, 0}, {0, 167.6f, 5.427e3f}, {0, 0, 0}, 1.024e26f, 1638, false, {.22f, .35f, .75f, 1})
    };
    ComputeAccelerations(objects);
    orbitTrails.resize(objects.size());

    int gridUpdateCounter = 0;

    while(!glfwWindowShouldClose(window)) {
        float now = glfwGetTime();
        deltaTime = now - lastFrame;
        lastFrame = now;

        // FPS
        fpsFrames++; fpsTimer += deltaTime;
        if (fpsTimer >= 0.5f) { currentFPS = fpsFrames/fpsTimer; fpsFrames=0; fpsTimer=0; }

        processInput(window);

        // --- IMGUI NEW FRAME ---
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // View matrix
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos+cameraFront, cameraUp);
        glUniformMatrix4fv(glGetUniformLocation(shader,"view"),1,GL_FALSE,glm::value_ptr(view));

        // Zoom
        if (fabs(zoomSpeed) > 0.0001f) {
            if (followPlanet >= 0) followOffset += cameraFront*(glm::length(followOffset)*zoomSpeed);
            else { float d=glm::length(cameraPos); cameraPos+=cameraFront*(d*zoomSpeed); }
            zoomSpeed *= 0.85f;
        }

        // Follow planet
        if (followPlanet >= 0 && followPlanet < (int)objects.size()) {
            glm::vec3 pp = (followPlanet==0) ? objects[0].getPosition()
                : objects[followPlanet].getPosition()-objects[0].getPosition();
            glm::vec3 pg = pp/(float)SCALE;
            float pr = (objects[followPlanet].getRadius()/(float)SCALE)*planetSizeMultiplier;
            pg.y += GetSpacetimeDepth(pg.x, pg.z, objects) + pr * 1.1f;
            if (glm::length(followOffset)<0.001f) followOffset = glm::vec3(0,pr*2,pr*5);
            cameraPos = pg + followOffset;

            // Sun-Focus mode: auto-orient camera to always look at the Sun
            if (followViewMode == 0 && followPlanet != 0) {
                float sunR = (objects[0].getRadius()/(float)SCALE)*planetSizeMultiplier;
                glm::vec3 sunPos = glm::vec3(0, GetSpacetimeDepth(0, 0, objects) + sunR * 1.1f, 0);
                cameraFront = glm::normalize(sunPos - cameraPos);
                // Update yaw/pitch to match so arrow keys work relative to Sun direction
                pitch_ = glm::degrees(asin(cameraFront.y));
                yaw_ = glm::degrees(atan2(cameraFront.z, cameraFront.x));
            }
            // Observer mode (followViewMode==1): camera direction stays user-controlled
        }

        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        UpdateCam(shader, cameraPos);
        glUseProgram(shader);
        glUniform3f(lightLoc, 0,0,0);
        glUniform1f(ambLoc, 0.08f);
        glUniform1f(timeLoc, now);

        // --- BACKGROUND NEBULA ---
        glDepthMask(GL_FALSE); // Wait behind everything
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_FRONT); // Render inside out so we see the interior
        GLint nebulaLoc = glGetUniformLocation(shader, "NEBULA");
        glUniform1i(nebulaLoc, 1);
        glUniform1i(glGetUniformLocation(shader,"isGrid"), 0);
        glUniform1i(glGetUniformLocation(shader,"GLOW"), 0);
        glm::mat4 bgm = glm::scale(glm::translate(glm::mat4(1), cameraPos), glm::vec3(50000.0f)); // Huge background sphere centered at camera
        glUniformMatrix4fv(modLoc, 1, GL_FALSE, glm::value_ptr(bgm));
        glBindVertexArray(unitSphereVAO);
        glDrawElements(GL_TRIANGLES, unitSphereVertexCount, GL_UNSIGNED_INT, 0);
        glDisable(GL_CULL_FACE);
        glEnable(GL_DEPTH_TEST);
        glUniform1i(nebulaLoc, 0);
        glDepthMask(GL_TRUE);

        // --- STARFIELD (multi-layer) ---
        glUniform1i(glGetUniformLocation(shader,"isGrid"),1);
        glUniform1i(glGetUniformLocation(shader,"GLOW"),0);
        glm::mat4 sm = glm::translate(glm::mat4(1), cameraPos);
        glUniformMatrix4fv(modLoc,1,GL_FALSE,glm::value_ptr(sm));
        for (auto& sl : starLayers) {
            glUniform4f(colLoc, sl.r, sl.g, sl.b, sl.a);
            glPointSize(sl.pointSize);
            glBindVertexArray(sl.VAO);
            glDrawArrays(GL_POINTS, 0, sl.count);
        }
        glBindVertexArray(0);

        // --- PHYSICS ---
        float totalDt = paused ? 0 : deltaTime * 86400 * baseTimeScale * simSpeedMultiplier;
        if (totalDt > 0) {
            // Sub-stepping to prevent Velocity Verlet integrator from exploding 
            // under the modified GR potential at high time-accelerations.
            int steps = std::max(1, (int)std::ceil(totalDt / 4000.0f)); 
            float dt = totalDt / steps;
            for (int s = 0; s < steps; ++s) {
                for (auto &o:objects) o.setVelocity(o.getVelocity()+0.5f*o.getAcceleration()*dt);
                for (auto &o:objects) o.setPosition(o.getPosition()+o.getVelocity()*dt);
                ComputeAccelerations(objects);
                for (auto &o:objects) o.setVelocity(o.getVelocity()+0.5f*o.getAcceleration()*dt);
            }
        }

        // --- SPACETIME GRID ---
        if (showSpacetimeGrid) {
            gridUpdateCounter++;
            if (gridUpdateCounter % 3 == 0 || spacetimeGridVertCount == 0)
                UpdateSpacetimeGrid(objects);
            glUniform1i(glGetUniformLocation(shader,"isGrid"),1);
            glUniform1i(glGetUniformLocation(shader,"GLOW"),0);
            glm::mat4 id(1); glUniformMatrix4fv(modLoc,1,GL_FALSE,glm::value_ptr(id));
            glUniform4f(colLoc, 0.15f,0.4f,0.6f,0.5f);
            glBindVertexArray(spacetimeGridVAO);
            glDrawArrays(GL_LINES,0,spacetimeGridVertCount);
            glBindVertexArray(0);
        }

        // --- ORBIT TRAILS ---
        trailFrameCounter++;
        if (!paused && trailFrameCounter%3==0) {
            for (int i=1;i<(int)objects.size();i++) {
                glm::vec3 p = (objects[i].getPosition()-objects[0].getPosition())/(float)SCALE;
                orbitTrails[i].push_back(p);
                if ((int)orbitTrails[i].size()>MAX_TRAIL_POINTS) orbitTrails[i].pop_front();
            }
        }
        if (showTrails) {
            glUniform1i(glGetUniformLocation(shader,"isGrid"),1);
            glUniform1i(glGetUniformLocation(shader,"GLOW"),0);
            glm::mat4 id(1); glUniformMatrix4fv(modLoc,1,GL_FALSE,glm::value_ptr(id));
            for (int i=1;i<(int)objects.size();i++) {
                if (orbitTrails[i].size()<2) continue;
                vector<float> tv;
                float objGr = (objects[i].getRadius()/(float)SCALE)*planetSizeMultiplier;
                for (auto& p : orbitTrails[i]) { 
                    float dp = p.y + GetSpacetimeDepth(p.x, p.z, objects) + objGr * 1.1f;
                    tv.push_back(p.x); tv.push_back(dp); tv.push_back(p.z); 
                    tv.push_back(0); tv.push_back(0); tv.push_back(0); 
                }
                glm::vec4 c=objects[i].getColor();
                glUniform4f(colLoc, c.r*0.5f,c.g*0.5f,c.b*0.5f,0.6f);
                glBindVertexArray(trailVAO); glBindBuffer(GL_ARRAY_BUFFER,trailVBO);
                glBufferData(GL_ARRAY_BUFFER,tv.size()*sizeof(float),tv.data(),GL_DYNAMIC_DRAW);
                glDrawArrays(GL_LINE_STRIP,0,orbitTrails[i].size());
            }
            glBindVertexArray(0);
        }

        // --- PLANETS ---
        for (auto &obj : objects) {
            glm::vec3 P = (&obj==&objects[0]) ? obj.getPosition() : obj.getPosition()-objects[0].getPosition();
            glm::vec3 gp = P/(float)SCALE;
            float gr = (obj.getRadius()/(float)SCALE)*planetSizeMultiplier;
            gp.y += GetSpacetimeDepth(gp.x, gp.z, objects) + gr * 1.1f;
            glm::mat4 m = glm::scale(glm::translate(glm::mat4(1),gp), glm::vec3(gr));
            glUniformMatrix4fv(modLoc,1,GL_FALSE,glm::value_ptr(m));
            glm::vec4 c=obj.getColor();
            glUniform4f(colLoc,c.r,c.g,c.b,c.a);
            glUniform1i(glGetUniformLocation(shader,"GLOW"), obj.isGlowing()?1:0);
            glUniform1i(glGetUniformLocation(shader,"isGrid"),0);
            glBindVertexArray(unitSphereVAO);
            glDrawElements(GL_TRIANGLES,unitSphereVertexCount,GL_UNSIGNED_INT,0);
        }

        // --- ON-SCREEN UI & HUD (ImGui) ---
        // 1. Planet Labels Overlay
        ImDrawList* drawList = ImGui::GetBackgroundDrawList();
        const ImU32 labelColor = IM_COL32(255, 255, 255, 230);
        glm::mat4 viewProj = proj * view;
        for (int i = 0; i < objects.size(); i++) {
            glm::vec3 P = (i == 0) ? objects[i].getPosition() : objects[i].getPosition() - objects[0].getPosition();
            glm::vec3 gp = P / (float)SCALE;
            float gr = (objects[i].getRadius()/(float)SCALE)*planetSizeMultiplier;
            gp.y += GetSpacetimeDepth(gp.x, gp.z, objects) + gr * 1.1f;
            // Project 3D GL coordinate to 2D NDC bounding box
            glm::vec4 ndc = viewProj * glm::vec4(gp, 1.0f);
            if (ndc.z > 0.0f) { // Only if in front of camera
                ndc /= ndc.w;
                // Convert to screen coordinates
                glm::vec2 screenPos = glm::vec2((ndc.x + 1.0f) * 0.5f * screenWidth, (1.0f - ndc.y) * 0.5f * screenHeight);
                float radiusPx = (objects[i].getRadius() / (float)SCALE) * planetSizeMultiplier * (screenHeight / (ndc.z * tan(glm::radians(45.0f)/2.0f)));

                // Calculate distance text
                double distMeters = glm::length(objects[i].getPosition() - objects[0].getPosition());
                double distMillionKm = distMeters / 1e9; // 1 billion meters = 1 million km
                char labelStr[64];
                if (i == 0) snprintf(labelStr, sizeof(labelStr), "SUN");
                else snprintf(labelStr, sizeof(labelStr), "%s\n%.1fM km", planetNames[i], distMillionKm);
                
                ImVec2 textSize = ImGui::CalcTextSize(labelStr);
                // Draw text slightly above planet
                drawList->AddText(ImVec2(screenPos.x - textSize.x * 0.5f, screenPos.y - radiusPx - textSize.y - 15.0f), labelColor, labelStr);
            }
        }

        // 2. Top-Right Telemetry HUD
        ImGui::SetNextWindowPos(ImVec2(screenWidth - 10.0f, 10.0f), ImGuiCond_Always, ImVec2(1.0f, 0.0f));
        ImGui::SetNextWindowBgAlpha(0.75f);
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove;
        if (ImGui::Begin("Telemetry HUD", NULL, window_flags))
        {
            ImGui::Text("=== TELEMETRY ===");
            ImGui::Separator();
            ImGui::Text("FPS: %.0f", currentFPS);
            ImGui::Text("Physics: %s", physicsMode==EINSTEIN?"Einstein (GR)":"Newtonial");
            
            // Calculate time flow definition
            double speedTotalSeconds = deltaTime * 86400 * baseTimeScale * simSpeedMultiplier; 
            double daysPerRealSecond = (speedTotalSeconds / deltaTime) / 86400.0;
            if (paused) ImGui::Text("Time: PAUSED");
            else ImGui::Text("Time: 1s = %.1f days", daysPerRealSecond);
            
            ImGui::Separator();
            ImGui::Text("--- SETTINGS ---");
            ImGui::SliderFloat("Base Time Scale", &baseTimeScale, 0.1f, 100.0f, "%.1f days / sec");

            ImGui::Text("Scale: %s", trueScaleMode?"True Scale":"Enhanced Visual");
            if (followPlanet >= 0)
                ImGui::Text("Camera: Following %s (%s)", planetNames[followPlanet], followViewNames[followViewMode]);
            else
                ImGui::Text("Camera: Free");
            ImGui::Separator();
            ImGui::Text("Controls: H for Help");
        }
        ImGui::End();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup ImGui
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window); glfwTerminate();
    return 0;
}
