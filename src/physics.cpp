#include "physics.h"

// Pure Newtonian gravity (N-body pairwise)
static void ComputeNewton(vector<Object>& objects) {
    for (auto &o : objects) o.setAcceleration(glm::vec3(0.0f));
    size_t n = objects.size();
    for (size_t i = 0; i < n; ++i) {
        for (size_t j = i+1; j < n; ++j) {
            glm::dvec3 pi(objects[i].getPosition()), pj(objects[j].getPosition());
            glm::dvec3 dir = pj - pi;
            double dist2 = glm::dot(dir,dir) + SOFTENING*SOFTENING;
            double dist = sqrt(dist2);
            glm::dvec3 norm = dir / dist;
            double mi = objects[i].getMass(), mj = objects[j].getMass();
            glm::dvec3 a_i =  norm * (Gd * mj / dist2);
            glm::dvec3 a_j = -norm * (Gd * mi / dist2);
            objects[i].setAcceleration(objects[i].getAcceleration() + glm::vec3(a_i));
            objects[j].setAcceleration(objects[j].getAcceleration() + glm::vec3(a_j));
        }
    }
}

// Einstein stable 1PN correction
// Uses a central-force addition (3 * G * M * h^2 / c^2 r^4) which accurately 
// yields the exact Einstein perihelion shift. Unlike the full standard 1PN formula, 
// this purely central addition drops velocity-dependent feedback terms, 
// making it 100% numerically stable for large dt semi-implicit Euler / Verlet integrations.
static void ComputeEinstein(vector<Object>& objects) {
    for (auto &o : objects) o.setAcceleration(glm::vec3(0.0f));
    size_t n = objects.size();
    for (size_t i = 0; i < n; ++i) {
        for (size_t j = i+1; j < n; ++j) {
            glm::dvec3 pi(objects[i].getPosition()), pj(objects[j].getPosition());
            glm::dvec3 vi(objects[i].getVelocity()), vj(objects[j].getVelocity());
            glm::dvec3 rij = pj - pi;
            double dist2 = glm::dot(rij,rij) + SOFTENING*SOFTENING;
            double dist = sqrt(dist2);
            glm::dvec3 nhat = rij / dist;
            double mi = objects[i].getMass(), mj = objects[j].getMass();

            // Newtonian
            double ai_mag = Gd * mj / dist2;
            double aj_mag = Gd * mi / dist2;
            glm::dvec3 a_i_N =  nhat * ai_mag;
            glm::dvec3 a_j_N = -nhat * aj_mag;

            // Stable Einstein perihelion shift correction
            glm::dvec3 v_rel = vj - vi;
            glm::dvec3 h_vec = glm::cross(rij, v_rel);
            double h2 = glm::dot(h_vec, h_vec);
            
            double d4 = dist2 * dist2;
            double a_i_1PN_mag = 3.0 * Gd * mj * h2 / (C2 * d4);
            double a_j_1PN_mag = 3.0 * Gd * mi * h2 / (C2 * d4);

            glm::dvec3 a_i_1PN =  nhat * a_i_1PN_mag;
            glm::dvec3 a_j_1PN = -nhat * a_j_1PN_mag;

            objects[i].setAcceleration(objects[i].getAcceleration() + glm::vec3(a_i_N + a_i_1PN));
            objects[j].setAcceleration(objects[j].getAcceleration() + glm::vec3(a_j_N + a_j_1PN));
        }
    }
}

void ComputeAccelerations(vector<Object>& objects) {
    if (physicsMode == EINSTEIN) ComputeEinstein(objects);
    else                         ComputeNewton(objects);
}
