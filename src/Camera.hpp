#pragma once

// GLM
#include <glm/glm.hpp>
#include <glm/ext.hpp>

// USUAL INCLUDES
#include <math.h>
#include "Transformation.hpp"

class Camera {
public:
#define CAMERA_TYPES_N 2
#define CAMERA_TYPES "Free\0Orbital\0"
    enum class Type {
        Free = 0,
        Orbital = 1,
    };

    Type m_type = Type::Free;
    glm::vec3 m_position = glm::vec3(1.0f, 1.0f, 1.0f);
    float m_translation_speed = 2.5f;

    glm::vec2 m_orientation = glm::vec2(-M_PI_4 * 0.5, 0.); // (pitch, yaw)
    float m_rotation_speed = 0.5f;

    float m_aspect_ratio = 1.f;
    float m_fovy = M_PI_2f;

    const glm::vec3 *m_center = &VEC_ZERO; // Only in oribtal type
    float m_distance_to_center = 5.f;      // Only in oribtal type
    float m_zoom_rate = 0.05f;             // Only in oribtal type

private:
    glm::vec3 m_front;
    glm::vec3 m_right;
    glm::vec3 m_real_up;

    glm::mat4 m_view;
    glm::mat4 m_projection;

    void updateData();

    bool updateInterface(float _deltaTime);
    void updateKeyboardInput(GLFWwindow *_window, float _deltaTime);
    void updateMouseInput(GLFWwindow *_window, float _deltaTime, const glm::vec2 &_cursor_vel, const glm::vec2 &_scroll, bool _disable_actions);

public:
    Camera() { updateData(); };

    void update(GLFWwindow *_window, float _deltaTime, const glm::vec2 &_cursor_vel, const glm::vec2 &_scroll);

    glm::vec3 getFront() const { return m_front; }
    glm::vec3 getRight() const { return m_right; }
    glm::vec3 getUp() const { return m_real_up; }

    glm::mat4 getViewMatrix() const { return m_view; }
    glm::mat4 getProjectionMatrix() const { return m_projection; }
};
