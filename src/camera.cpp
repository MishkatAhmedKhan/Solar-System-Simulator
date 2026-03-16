#include "camera.h"
#include "scene.h"

void updateCameraFront() {
    glm::vec3 f;
    f.x = cos(glm::radians(yaw_)) * cos(glm::radians(pitch_));
    f.y = sin(glm::radians(pitch_));
    f.z = sin(glm::radians(yaw_)) * cos(glm::radians(pitch_));
    cameraFront = glm::normalize(f);
}

void scroll_callback(GLFWwindow*, double, double yo) {
    zoomSpeed += (float)yo * 0.15f;
}

void mouse_callback(GLFWwindow*, double xp, double yp) {
    if (!freeLookMode) return;
    if (firstMouse) { lastMouseX=xp; lastMouseY=yp; firstMouse=false; return; }
    float xo = (float)(xp-lastMouseX)*mouseSensitivity;
    float yo = (float)(lastMouseY-yp)*mouseSensitivity;
    lastMouseX=xp; lastMouseY=yp;
    yaw_ += xo; pitch_ += yo;
    if (pitch_ > 89) pitch_=89; if (pitch_ < -89) pitch_=-89;
    updateCameraFront();
}

void UpdateCam(GLuint prog, glm::vec3 pos) {
    glUseProgram(prog);
    glm::mat4 v = glm::lookAt(pos, pos+cameraFront, cameraUp);
    glUniformMatrix4fv(glGetUniformLocation(prog,"view"),1,GL_FALSE,glm::value_ptr(v));
}

void processInput(GLFWwindow* w) {
    float dist = glm::length(cameraPos);
    float speed = dist * 0.8f * deltaTime;
    if (glfwGetKey(w,GLFW_KEY_LEFT_SHIFT)==GLFW_PRESS) speed *= 3;

    if (followPlanet < 0) {
        if (glfwGetKey(w,GLFW_KEY_W)==GLFW_PRESS) cameraPos += speed*cameraFront;
        if (glfwGetKey(w,GLFW_KEY_S)==GLFW_PRESS) cameraPos -= speed*cameraFront;
        if (glfwGetKey(w,GLFW_KEY_A)==GLFW_PRESS) cameraPos -= glm::normalize(glm::cross(cameraFront,cameraUp))*speed;
        if (glfwGetKey(w,GLFW_KEY_D)==GLFW_PRESS) cameraPos += glm::normalize(glm::cross(cameraFront,cameraUp))*speed;
        if (glfwGetKey(w,GLFW_KEY_SPACE)==GLFW_PRESS) cameraPos += speed*cameraUp;
        if (glfwGetKey(w,GLFW_KEY_LEFT_CONTROL)==GLFW_PRESS) cameraPos -= speed*cameraUp;
    } else {
        float fs = 50*deltaTime;
        if (glfwGetKey(w,GLFW_KEY_LEFT_SHIFT)==GLFW_PRESS) fs *= 3;
        if (glfwGetKey(w,GLFW_KEY_W)==GLFW_PRESS) followOffset += fs*cameraFront;
        if (glfwGetKey(w,GLFW_KEY_S)==GLFW_PRESS) followOffset -= fs*cameraFront;
        if (glfwGetKey(w,GLFW_KEY_A)==GLFW_PRESS) followOffset -= glm::normalize(glm::cross(cameraFront,cameraUp))*fs;
        if (glfwGetKey(w,GLFW_KEY_D)==GLFW_PRESS) followOffset += glm::normalize(glm::cross(cameraFront,cameraUp))*fs;
        if (glfwGetKey(w,GLFW_KEY_SPACE)==GLFW_PRESS) followOffset += fs*cameraUp;
        if (glfwGetKey(w,GLFW_KEY_LEFT_CONTROL)==GLFW_PRESS) followOffset -= fs*cameraUp;
    }

    // Arrow keys
    float rs = 80*deltaTime;
    if (glfwGetKey(w,GLFW_KEY_LEFT)==GLFW_PRESS)  yaw_ -= rs;
    if (glfwGetKey(w,GLFW_KEY_RIGHT)==GLFW_PRESS) yaw_ += rs;
    if (glfwGetKey(w,GLFW_KEY_UP)==GLFW_PRESS)    pitch_ += rs;
    if (glfwGetKey(w,GLFW_KEY_DOWN)==GLFW_PRESS)  pitch_ -= rs;
    if (pitch_>89) pitch_=89; if (pitch_<-89) pitch_=-89;
    if (glfwGetKey(w,GLFW_KEY_LEFT)==GLFW_PRESS||glfwGetKey(w,GLFW_KEY_RIGHT)==GLFW_PRESS||
        glfwGetKey(w,GLFW_KEY_UP)==GLFW_PRESS||glfwGetKey(w,GLFW_KEY_DOWN)==GLFW_PRESS)
        updateCameraFront();

    // Zoom
    if (glfwGetKey(w,GLFW_KEY_EQUAL)==GLFW_PRESS) zoomSpeed += 2*deltaTime;
    if (glfwGetKey(w,GLFW_KEY_MINUS)==GLFW_PRESS) zoomSpeed -= 2*deltaTime;

    // Planet follow (0-8)
    int nk[]={GLFW_KEY_0,GLFW_KEY_1,GLFW_KEY_2,GLFW_KEY_3,GLFW_KEY_4,GLFW_KEY_5,GLFW_KEY_6,GLFW_KEY_7,GLFW_KEY_8};
    for (int i=0;i<9;i++) {
        if (glfwGetKey(w,nk[i])==GLFW_PRESS && !numberKeyWasPressed[i]) {
            numberKeyWasPressed[i]=true;
            if (followPlanet==i) { followPlanet=-1; followOffset=glm::vec3(0); cerr<<"[Camera] Free\n"; }
            else { followPlanet=i; followOffset=glm::vec3(0,3,10); cerr<<"[Camera] Following "<<planetNames[i]<<endl; }
        }
        if (glfwGetKey(w,nk[i])==GLFW_RELEASE) numberKeyWasPressed[i]=false;
    }
    // Backtick cycle
    if (glfwGetKey(w,GLFW_KEY_GRAVE_ACCENT)==GLFW_PRESS && !graveKeyWasPressed) {
        graveKeyWasPressed=true; followPlanet=(followPlanet+1)%9;
        followOffset=glm::vec3(0,3,10); cerr<<"[Camera] "<<planetNames[followPlanet]<<endl;
    }
    if (glfwGetKey(w,GLFW_KEY_GRAVE_ACCENT)==GLFW_RELEASE) graveKeyWasPressed=false;

    // Sim speed
    if (glfwGetKey(w,GLFW_KEY_RIGHT_BRACKET)==GLFW_PRESS) simSpeedMultiplier=min(simSpeedMultiplier*(1+1.5f*deltaTime),100.0f);
    if (glfwGetKey(w,GLFW_KEY_LEFT_BRACKET)==GLFW_PRESS)  simSpeedMultiplier=max(simSpeedMultiplier*(1-1.5f*deltaTime),0.01f);

    // Pause
    if (glfwGetKey(w,GLFW_KEY_P)==GLFW_PRESS && !pKeyWasPressed) { paused=!paused; pKeyWasPressed=true; cerr<<"[Sim] "<<(paused?"PAUSED":"RESUMED")<<endl; }
    if (glfwGetKey(w,GLFW_KEY_P)==GLFW_RELEASE) pKeyWasPressed=false;

    // Trails
    if (glfwGetKey(w,GLFW_KEY_O)==GLFW_PRESS && !oKeyWasPressed) { showTrails=!showTrails; oKeyWasPressed=true; }
    if (glfwGetKey(w,GLFW_KEY_O)==GLFW_RELEASE) oKeyWasPressed=false;

    // Follow view mode toggle (V)
    if (glfwGetKey(w,GLFW_KEY_V)==GLFW_PRESS && !vKeyWasPressed) {
        vKeyWasPressed=true;
        followViewMode = (followViewMode + 1) % 2;
        cerr << "[View] " << followViewNames[followViewMode]
             << (followViewMode==0 ? " (camera locks onto Sun)" : " (free look from planet surface)")
             << endl;
    }
    if (glfwGetKey(w,GLFW_KEY_V)==GLFW_RELEASE) vKeyWasPressed=false;

    // Physics mode toggle (G)
    if (glfwGetKey(w,GLFW_KEY_G)==GLFW_PRESS && !gKeyWasPressed) {
        gKeyWasPressed=true;
        physicsMode = (physicsMode==NEWTON) ? EINSTEIN : NEWTON;
        showSpacetimeGrid = (physicsMode==EINSTEIN);
        cerr<<"[Physics] "<<(physicsMode==EINSTEIN?"EINSTEIN (GR 1PN)":"NEWTON")<<endl;
    }
    if (glfwGetKey(w,GLFW_KEY_G)==GLFW_RELEASE) gKeyWasPressed=false;

    // Scale
    if (glfwGetKey(w,GLFW_KEY_T)==GLFW_PRESS && !tKeyWasPressed) {
        trueScaleMode=!trueScaleMode; planetSizeMultiplier=trueScaleMode?1:200;
        tKeyWasPressed=true; cerr<<"[Scale] "<<planetSizeMultiplier<<"x\n";
    }
    if (glfwGetKey(w,GLFW_KEY_T)==GLFW_RELEASE) tKeyWasPressed=false;

    // Free-look
    if (glfwGetKey(w,GLFW_KEY_F)==GLFW_PRESS && !fKeyWasPressed) {
        freeLookMode=!freeLookMode; fKeyWasPressed=true;
        if (freeLookMode) { firstMouse=true; glfwSetInputMode(w,GLFW_CURSOR,GLFW_CURSOR_DISABLED); cerr<<"[Camera] Free-look ON\n"; }
        else { glfwSetInputMode(w,GLFW_CURSOR,GLFW_CURSOR_NORMAL); cerr<<"[Camera] Free-look OFF\n"; }
    }
    if (glfwGetKey(w,GLFW_KEY_F)==GLFW_RELEASE) fKeyWasPressed=false;

    // Reset
    if (glfwGetKey(w,GLFW_KEY_R)==GLFW_PRESS) {
        followPlanet=-1; followOffset=glm::vec3(0);
        cameraPos=glm::vec3(0,100,800); yaw_=-90; pitch_=0; updateCameraFront();
    }

    // Help
    if (glfwGetKey(w,GLFW_KEY_H)==GLFW_PRESS) printHelp();

    // Quit
    if (glfwGetKey(w,GLFW_KEY_ESCAPE)==GLFW_PRESS) glfwSetWindowShouldClose(w,true);
}
