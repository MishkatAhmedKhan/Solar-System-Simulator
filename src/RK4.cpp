#include <bits/stdc++.h> 
#include <GL/glew.h>
#include <GLFW/glfw3.h> 
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cmath> 

using namespace std;

const char* vertexShaderSource = R"glsl(
#version 330 core
layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNormal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform vec3 lightPos; // world-space light position

out float lightIntensity;
out vec3 vNormal;
out vec3 vWorldPos;

void main() {
    vec4 worldPosition = model * vec4(aPos, 1.0);
    vWorldPos = worldPosition.xyz;

    // normal -> world space using inverse-transpose of model
    mat3 normalMatrix = mat3(transpose(inverse(model)));
    vNormal = normalize(normalMatrix * aNormal);

    vec3 dirToLight = normalize(lightPos - vWorldPos);
    lightIntensity = max(dot(vNormal, dirToLight), 0.0);

    gl_Position = projection * view * worldPosition;
})glsl";

const char* fragmentShaderSource = R"glsl(
#version 330 core
in float lightIntensity;
in vec3 vNormal;
in vec3 vWorldPos;

out vec4 FragColor;
uniform vec4 objectColor;
uniform bool isGrid;
uniform bool GLOW;
uniform float ambient; // e.g. 0.1

void main() {
    if (isGrid) {
        FragColor = objectColor;
        return;
    }

    // simple lambert + ambient
    float lambert = max(lightIntensity, 0.0);
    float intensity = clamp(ambient + lambert * (1.0 - ambient), 0.0, 1.0);

    vec3 color = objectColor.rgb * intensity;

    if (GLOW) {
        // glow: additively boost color, but a reasonable scale
        vec3 glowColor = objectColor.rgb * 4.0; // tune 2.0..8.0
        // additive blend will be used by enabling blending in GL and drawing glow pass
        FragColor = vec4(glowColor, objectColor.a);
    } else {
        FragColor = vec4(color, objectColor.a);
    }
})glsl";

glm::vec3 cameraPos   = glm::vec3(0.0f, 0.0f,  1.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp    = glm::vec3(0.0f, 1.0f,  0.0f);

float screenWidth = 1600.0f;
float screenHeight = 900.0f; 
const float G = 6.67430e-11; // gravitational constant

float deltaTime = 0.0;
float lastFrame = 0.0;

float sizeRatio = 30000.0f;

GLFWwindow* StartGLU();
GLuint CreateShaderProgram(const char* vertexSource, const char* fragmentSource);
void CreateVBOVAO(GLuint& VAO, GLuint& VBO, const float* vertices, size_t vertexCount);
void UpdateCam(GLuint shaderProgram, glm::vec3 cameraPos);
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

void mouse_callback(GLFWwindow* window, double xpos, double ypos);
glm::vec3 sphericalToCartesian(float r, float theta, float phi);
void DrawGrid(GLuint shaderProgram, GLuint gridVAO, size_t vertexCount);

class Object {
    private:
        glm::vec3 position;
        glm::vec3 velocity;
        float density;
        float radius; 
        float mass;

        glm::vec3 LastPos = position;
        bool glow;

    public:
        GLuint VAO, VBO;
        size_t vertexCount;

        Object(glm::vec3 pos, glm::vec3 vel, float r, float m, float d=3344, bool g=false) 
            : position(pos), velocity(vel), mass(m), density(d), glow(g) {
                radius = pow(((3 * mass/density)/(4 * 3.14159265359)), (1.0f/3.0f)) / sizeRatio;

                vector<float> vertices = draw(200);
                vertexCount = vertices.size();

                CreateVBOVAO(VAO, VBO, vertices.data(), vertexCount);
            }
        
        
        glm::vec3 getPosition() const { return position; }
        glm::vec3 getVelocity() const { return velocity; }
        float getRadius() const { return radius; }
        float getMass() const { return mass; }
        float getDensity() const { return density; }
        bool isGlowing() const { return glow; }

        void setPosition(const glm::vec3& pos) { position = pos; }
        void setVelocity(const glm::vec3& vel) { velocity = vel; }
        void setRadius(float r) { radius = r; }
        void setMass(float m) { mass = m; }
        void setDensity(float d) { density = d; }
        void setGlow(bool g) { glow = g; }
        
        vector<float> draw(int resolution) {
            vector<float> vertices;
            const float PI = 3.14159265359f;

            // Latitude: 0 → π
            for (int i = 0; i < resolution; ++i) {
                float theta1 = (float)i / resolution * PI;
                float theta2 = (float)(i + 1) / resolution * PI;

                // Longitude: 0 → 2π
                for (int j = 0; j < resolution; ++j) {
                    float phi1 = (float)j / resolution * 2.0f * PI;
                    float phi2 = (float)(j + 1) / resolution * 2.0f * PI;

                    // 4 corner points on the sphere surface (unit sphere)
                    glm::vec3 p1(sphericalToCartesian(radius, theta1, phi1));
                    glm::vec3 p2(sphericalToCartesian(radius, theta2, phi1));
                    glm::vec3 p3(sphericalToCartesian(radius, theta2, phi2));
                    glm::vec3 p4(sphericalToCartesian(radius, theta1, phi2));

                    // Each small rectangle → 2 triangles
                    auto pushVertex = [&](const glm::vec3 &pos) {
                        // position
                        vertices.push_back(pos.x);
                        vertices.push_back(pos.y);
                        vertices.push_back(pos.z);
                        // normal (same as normalized position for a sphere)
                        vertices.push_back(pos.x);
                        vertices.push_back(pos.y);
                        vertices.push_back(pos.z);
                    };

                    // Triangle 1
                    pushVertex(p1);
                    pushVertex(p2);
                    pushVertex(p3);

                    // Triangle 2
                    pushVertex(p1);
                    pushVertex(p3);
                    pushVertex(p4);
                }
            }

            return vertices;
        }

        void checkCollision(vector<Object>& objects, float restitution) {
            for (auto &other : objects) {
                if (&other == this) continue;
                if (&other < this) continue; // avoid duplicate checks

                // Center vector and distance
                glm::vec3 diff = other.position - position;
                float dist = glm::length(diff);
                float minDist = radius + other.radius;

                if (dist >= minDist) continue; // no collision or identical centers

                if (dist < 1e-6f) {
                    diff = glm::vec3(1e-3f, 0.0f, 0.0f);
                    dist = 1e-3f;
                }

                // Normal direction (from this → other)
                glm::vec3 n = diff / dist;

                // ------------------------------
                // 1) Positional correction
                // ------------------------------
                float overlap = minDist - dist;
                if (overlap > 0.0f) {
                    float m1 = mass, m2 = other.mass;
                    float total = m1 + m2;
                    if (total <= 0.0f) total = 1.0f;

                    const float percent = 0.8f; // push ratio (0.2–0.8)
                    float corr = overlap * percent;

                    // Push proportionally to inverse mass
                    glm::vec3 correction = corr * n;
                    position -= correction * (m2 / total);
                    other.position += correction * (m1 / total);
                }

                // ------------------------------
                // 2) Velocity update (elastic + restitution)
                // ------------------------------

                // Relative velocity
                glm::vec3 relVel = other.velocity - velocity;

                // Normal velocity component
                float velAlongNormal = glm::dot(relVel, n);
                if (velAlongNormal > 0.0f) continue; // moving apart, skip

                // Compute impulse scalar
                float m1 = mass, m2 = other.mass;
                float total = m1 + m2;
                if (total <= 0.0f) total = 1.0f;

                float j = -(1.0f + restitution) * velAlongNormal;
                j /= (1.0f / m1 + 1.0f / m2);

                // Apply impulse
                glm::vec3 impulse = j * n;
                velocity -= impulse / m1;
                other.velocity += impulse / m2;
            }
        }
    };

    // vector<Object> objects = {};

    GLuint gridVAO, gridVBO;

int main() {
    GLFWwindow* window = StartGLU();
    GLuint shaderProgram = CreateShaderProgram(vertexShaderSource, fragmentShaderSource);

    GLint lightPosLoc = glGetUniformLocation(shaderProgram, "lightPos");
    GLint ambientLoc  = glGetUniformLocation(shaderProgram, "ambient");

    GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
    GLint objectColorLoc = glGetUniformLocation(shaderProgram, "objectColor");
    glUseProgram(shaderProgram);

    //projection matrix
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 750000.0f);
    GLint projectionLoc = glGetUniformLocation(shaderProgram, "projection");
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
    cameraPos = glm::vec3(0.0f, 1000.0f, 5000.0f);

    


    if (!window) return -1;

    // Set clear color (black) 
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    float radius = 50.0f;
    int res = 100; // circle resolution

    vector<Object> objects {
        Object(glm::vec3(400.0f, 300.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), 50000.0f, 5.972e24, 3344, true)// Earth-like
    };

    vector<float> position = {screenWidth / 2.0f, screenHeight / 2.0f};
    pair<float, float> velocity = {0.0f, 0.0f};

    const float gravity = 9.81; 
    const float dt = 1.0 / 20; // fixed timestep 
    const float restitution = .90; // energy loss on bounce 
    const float drag = 0.1; // simple linear drag coefficient

    while (!glfwWindowShouldClose(window)) {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        UpdateCam(shaderProgram, cameraPos);
        
        glUseProgram(shaderProgram);
        glm::vec3 lightPos(0.0f, 3.0f, 5.0f);
        glUniform3f(lightPosLoc, lightPos.x, lightPos.y, lightPos.z);
        glUniform1f(ambientLoc, 0.12f);
        glUniform4f(objectColorLoc, 1.0f, 1.0f, 1.0f, 0.25f);
        
        for(auto &obj : objects) {
            glUniform4f(objectColorLoc, 1.0f, 0.0f, 0.0f, 1.0f);

            for(auto &obj2 : objects) {
                if(&obj2 == &obj) continue;

                float dx = obj2.getPosition()[0] - obj.getPosition()[0];
                float dy = obj2.getPosition()[1] - obj.getPosition()[1];
                float dz = obj2.getPosition()[2] - obj.getPosition()[2];
                float dist = sqrt(dx*dx + dy*dy + dz*dz);

                if(dist < 1e-6f) continue; // avoid singularity

                if(dist > 0.0f) {
                    vector<float> direction = {dx / dist, dy / dist, dz / dist}; // normalized direction vector
                    dist *= 1000;
                    float Gforce = (G * obj.getMass() * obj2.getMass()) / (dist * dist); // gravitational force magnitude

                    float acc1 = Gforce / obj.getMass(); // a = F/m
                    vector<float> acc = {direction[0] * acc1, direction[1]*acc1, direction[2]*acc1};

                }

            }

            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, obj.getPosition());
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glUniform1i(glGetUniformLocation(shaderProgram, "isGrid"), 0); // Not grid
            if(obj.isGlowing())
                glUniform1i(glGetUniformLocation(shaderProgram, "GLOW"), 1);
            else
                glUniform1i(glGetUniformLocation(shaderProgram, "GLOW"), 0);

            glBindVertexArray(obj.VAO);
            glDrawArrays(GL_TRIANGLES, 0, obj.vertexCount / 6);

        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

GLFWwindow* StartGLU() {
    if (!glfwInit()) {
        cout << "Failed to initialize GLFW, panic" << endl;
        return nullptr;
    }
    GLFWwindow* window = glfwCreateWindow(800, 600, "3D_TEST", NULL, NULL);
    if (!window) {
        cerr << "Failed to create GLFW window." << endl;
        glfwTerminate();
        return nullptr;
    }
    glfwMakeContextCurrent(window);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        cerr << "Failed to initialize GLEW." << endl;
        glfwTerminate();
        return nullptr;
    }

    glEnable(GL_DEPTH_TEST);
    glViewport(0, 0, 800, 600);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    return window;
}

GLuint CreateShaderProgram(const char* vertexSource, const char* fragmentSource) {
    // Vertex shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexSource, nullptr);
    glCompileShader(vertexShader);

    GLint success;
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
        cerr << "Vertex shader compilation failed: " << infoLog << endl;
    }

    // Fragment shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentSource, nullptr);
    glCompileShader(fragmentShader);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
        cerr << "Fragment shader compilation failed: " << infoLog << endl;
    }

    // Shader program
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
        cerr << "Shader program linking failed: " << infoLog << endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

void CreateVBOVAO(GLuint& VAO, GLuint& VBO, const float* vertices, size_t vertexCount) {
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertexCount * sizeof(float), vertices, GL_STATIC_DRAW);

    // position (layout 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // normal (layout 1)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

void UpdateCam(GLuint shaderProgram, glm::vec3 cameraPos) {
    glUseProgram(shaderProgram);
    glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
    GLint viewLoc = glGetUniformLocation(shaderProgram, "view");
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
}

// void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
//     float cameraSpeed = 10000.0f * deltaTime;
//     bool shiftPressed = (mods & GLFW_MOD_SHIFT) != 0;
//     Object& lastObj = objs[objs.size() - 1];
    

//     if (glfwGetKey(window, GLFW_KEY_W)==GLFW_PRESS){
//         cameraPos += cameraSpeed * cameraFront;
//     }
//     if (glfwGetKey(window, GLFW_KEY_S)==GLFW_PRESS){
//         cameraPos -= cameraSpeed * cameraFront;
//     }

//     if (glfwGetKey(window, GLFW_KEY_A)==GLFW_PRESS){
//         cameraPos -= cameraSpeed * glm::normalize(glm::cross(cameraFront, cameraUp));
//     }
//     if (glfwGetKey(window, GLFW_KEY_D)==GLFW_PRESS){
//         cameraPos += cameraSpeed * glm::normalize(glm::cross(cameraFront, cameraUp));
//     }

//     if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS){
//         cameraPos += cameraSpeed * cameraUp;
//     }
//     if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS){
//         cameraPos -= cameraSpeed * cameraUp;
//     }

//     if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS){
//         pause = true;
//     }
//     if (glfwGetKey(window, GLFW_KEY_K) == GLFW_RELEASE){
//         pause = false;
//     }
    
//     if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS){
//         glfwTerminate();
//         glfwWindowShouldClose(window);
//         running = false;
//     }

//     // init arrows pos up down left right
//     if(!objs.empty() && objs[objs.size() - 1].Initalizing){
//         if (key == GLFW_KEY_UP && (action == GLFW_PRESS || action == GLFW_REPEAT)){
//             if (!shiftPressed) {
//                 objs[objs.size()-1].position[1] += objs[objs.size() - 1].radius * 0.2;
//             }
//         };
//         if (key == GLFW_KEY_DOWN && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
//             if (!shiftPressed) {
//                 objs[objs.size()-1].position[1] -= objs[objs.size() - 1].radius * 0.2;
//             }
//         }
//         if(key == GLFW_KEY_RIGHT && (action == GLFW_PRESS || action == GLFW_REPEAT)){
//             objs[objs.size()-1].position[0] += objs[objs.size() - 1].radius * 0.2;
//         };
//         if(key == GLFW_KEY_LEFT && (action == GLFW_PRESS || action == GLFW_REPEAT)){
//             objs[objs.size()-1].position[0] -= objs[objs.size() - 1].radius * 0.2;
//         };
//         if (key == GLFW_KEY_UP && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
//             objs[objs.size()-1].position[2] += objs[objs.size() - 1].radius * 0.2;
//         };

//         if (key == GLFW_KEY_DOWN && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
//             objs[objs.size()-1].position[2] -= objs[objs.size() - 1].radius * 0.2;
//         }
//     };
    
// };

// void mouse_callback(GLFWwindow* window, double xpos, double ypos) {

//     float xoffset = xpos - lastX;
//     float yoffset = lastY - ypos; 
//     lastX = xpos;
//     lastY = ypos;

//     float sensitivity = 0.1f;
//     xoffset *= sensitivity;
//     yoffset *= sensitivity;

//     yaw += xoffset;
//     pitch += yoffset;

//     if(pitch > 89.0f) pitch = 89.0f;
//     if(pitch < -89.0f) pitch = -89.0f;

//     glm::vec3 front;
//     front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
//     front.y = sin(glm::radians(pitch));
//     front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
//     cameraFront = glm::normalize(front);
// }

// void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods){
//     if (button == GLFW_MOUSE_BUTTON_LEFT){
//         if (action == GLFW_PRESS){
//             objs.emplace_back(glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.0f, 0.0f, 0.0f), initMass);
//             objs[objs.size()-1].Initalizing = true;
//         };
//         if (action == GLFW_RELEASE){
//             objs[objs.size()-1].Initalizing = false;
//             objs[objs.size()-1].Launched = true;
//         };
//     };
//     if (!objs.empty() && button == GLFW_MOUSE_BUTTON_RIGHT && objs[objs.size()-1].Initalizing) {
//         if (action == GLFW_PRESS || action == GLFW_REPEAT) {
//             objs[objs.size()-1].mass *= 1.2;}
//             cout<<"MASS: "<<objs[objs.size()-1].mass<<endl;
//     }
// };

// void scroll_callback(GLFWwindow* window, double xoffset, double yoffset){
//     float cameraSpeed = 250000.0f * deltaTime;
//     if(yoffset>0){
//         cameraPos += cameraSpeed * cameraFront;
//     } else if(yoffset<0){
//         cameraPos -= cameraSpeed * cameraFront;
//     }
// }

glm::vec3 sphericalToCartesian(float r, float theta, float phi){
    float x = r * sin(theta) * cos(phi);
    float y = r * cos(theta);
    float z = r * sin(theta) * sin(phi);
    return glm::vec3(x, y, z);
};

void DrawGrid(GLuint shaderProgram, GLuint gridVAO, size_t vertexCount) {
    glUseProgram(shaderProgram);
    glm::mat4 model = glm::mat4(1.0f); // Identity matrix for the grid
    GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    glBindVertexArray(gridVAO);
    glPointSize(5.0f);
    glDrawArrays(GL_LINES, 0, vertexCount / 3);
    glBindVertexArray(0);
}

vector<float> CreateGridVertices(float size, int divisions, const vector<Object>& objs) {
    vector<float> vertices;
    float step = size / divisions;
    float halfSize = size / 2.0f;

    // x axis
    for (int yStep = 3; yStep <= 3; ++yStep) {
        float y = -halfSize*0.3f + yStep * step;
        for (int zStep = 0; zStep <= divisions; ++zStep) {
            float z = -halfSize + zStep * step;
            for (int xStep = 0; xStep < divisions; ++xStep) {
                float xStart = -halfSize + xStep * step;
                float xEnd = xStart + step;
                vertices.push_back(xStart); vertices.push_back(y); vertices.push_back(z);
                vertices.push_back(xEnd);   vertices.push_back(y); vertices.push_back(z);
            }
        }
    }
    // zaxis
    for (int xStep = 0; xStep <= divisions; ++xStep) {
        float x = -halfSize + xStep * step;
        for (int yStep = 3; yStep <= 3; ++yStep) {
            float y = -halfSize*0.3f + yStep * step;
            for (int zStep = 0; zStep < divisions; ++zStep) {
                float zStart = -halfSize + zStep * step;
                float zEnd = zStart + step;
                vertices.push_back(x); vertices.push_back(y); vertices.push_back(zStart);
                vertices.push_back(x); vertices.push_back(y); vertices.push_back(zEnd);
            }
        }
    }

    

    return vertices;
}

// vector<float> UpdateGridVertices(vector<float> vertices, const vector<Object>& objs){
    
//     // centre of mass calc
//     float totalMass = 0.0f;
//     float comY = 0.0f;
//     for (const auto& obj : objs) {
//         if (obj.Initalizing) continue;
//         comY += obj.mass * obj.position.y;
//         totalMass += obj.mass;
//     }
//     if (totalMass > 0) comY /= totalMass;
    
//     float originalMaxY = -numeric_limits<float>::infinity();
//     for (int i = 0; i < vertices.size(); i += 3) {
//         originalMaxY = max(originalMaxY, vertices[i+1]);
//     }

//     float verticalShift = comY - originalMaxY;
//     cout<<"vertical shift: "<<verticalShift<<" |         comY: "<<comY<<"|            originalmaxy: "<<originalMaxY<<endl;


//     for (int i = 0; i < vertices.size(); i += 3) {

//         // mass bending space
//         glm::vec3 vertexPos(vertices[i], vertices[i+1], vertices[i+2]);
//         glm::vec3 totalDisplacement(0.0f);
//         for (const auto& obj : objs) {
//             //f (obj.Initalizing) continue;

//             glm::vec3 toObject = obj.GetPos() - vertexPos;
//             float distance = glm::length(toObject);
//             float distance_m = distance * 1000.0f;
//             float rs = (2*G*obj.mass)/(c*c);

//             float dz = 2 * sqrt(rs * (distance_m - rs));
//             totalDisplacement.y += dz * 2.0f;
//         }
//         vertices[i+1] = totalDisplacement.y + -abs(verticalShift);
//     }

//     return vertices;
// }




vector<Object> objects {
        Object(glm::vec3(400.0f, 300.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), 50000.0f, 5.972e24, 3344, true)// Earth-like
    };