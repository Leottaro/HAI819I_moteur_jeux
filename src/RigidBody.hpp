#pragma once

// GLM
#include <glm/ext.hpp>
#include <glm/glm.hpp>

#include <vector>
#include <iostream>

#define G 6.6742e-11f

struct RigidBody {
    glm::vec3 m_pos = glm::vec3(0.f, 0.f, 0.f);
    glm::vec3 m_vel = glm::vec3(0.f, 0.f, 0.f);
    float m_weight = 500.f;
    float m_volume = 1.f;
    float m_friction = 0.5f;
    float m_restitution = 0.5f;
    float m_drag = 1.05f;

    void update(float _deltaTime, const std::vector<glm::vec3> &_forces) {
        glm::vec3 m_accel = glm::vec3(0.f, 0.f, 0.f);
        for (const glm::vec3 &force : _forces) {
            m_accel += force;
        }
        m_accel /= m_weight;

        m_vel += _deltaTime * m_accel;
        m_pos += _deltaTime * m_vel;
    }

    void bounce(float _static_friction, const glm::vec3 &_normal) {
        float v_dot_n = glm::dot(m_vel, _normal);
        glm::vec3 v_normal = v_dot_n * _normal;
        glm::vec3 v_tangent = m_vel - v_normal;
        v_tangent *= 1.f - m_friction;
        if (glm::length(v_tangent) < _static_friction * glm::length(v_normal)) {
            v_tangent *= 0.f;
        }
        v_normal *= -m_restitution;
        if (glm::length(v_normal) < 1.f) {
            v_normal *= 0.f;
        }
        m_vel = v_normal + v_tangent;
    }
};
