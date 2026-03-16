#pragma once
#include "globals.h"

class Object {
    glm::vec3 position, velocity, acceleration;
    float density, radius, mass;
    bool glow;
    glm::vec4 color;
public:
    Object(glm::vec3 pos, glm::vec3 vel, glm::vec3 acc, float m, float d,
           bool g=false, glm::vec4 col=glm::vec4(1.0f))
        : position(pos), velocity(vel), acceleration(acc),
          mass(m), density(d), glow(g), color(col) {
        radius = pow((3*mass/density)/(4*3.14159265359), 1.0f/3.0f);
    }
    glm::vec3 getPosition() const { return position; }
    glm::vec3 getVelocity() const { return velocity; }
    glm::vec3 getAcceleration() const { return acceleration; }
    float getRadius() const { return radius; }
    float getMass() const { return mass; }
    float getDensity() const { return density; }
    bool  isGlowing() const { return glow; }
    glm::vec4 getColor() const { return color; }
    void setPosition(const glm::vec3& p) { position = p; }
    void setVelocity(const glm::vec3& v) { velocity = v; }
    void setAcceleration(const glm::vec3& a) { acceleration = a; }
    void setRadius(float r) { radius = r; }
    void setMass(float m) { mass = m; }
    void setGlow(bool g) { glow = g; }
    void setColor(const glm::vec4& c) { color = c; }
};
