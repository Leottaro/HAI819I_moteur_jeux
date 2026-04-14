// IMGUI
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

// USUAL INCLUDES
#include "Camera.hpp"
#include <iostream>

void Camera::Frustum::updatePlanes(Camera *_camera) {
    float far_height = _camera->m_near_far[1] * std::tan(_camera->m_fovy * 0.5f);
    float far_width = far_height * _camera->m_aspect_ratio;

    glm::vec3 left_normal = glm::normalize(glm::cross(
        _camera->m_front * _camera->m_near_far[1] - _camera->m_right * far_width,
        _camera->m_real_up));
    m_left = glm::vec4(left_normal, -glm::dot(left_normal, _camera->m_position));

    glm::vec3 right_normal = glm::normalize(glm::cross(
        _camera->m_real_up,
        _camera->m_front * _camera->m_near_far[1] + _camera->m_right * far_width));
    m_right = glm::vec4(right_normal, -glm::dot(right_normal, _camera->m_position));

    glm::vec3 bottom_normal = glm::normalize(glm::cross(
        _camera->m_right,
        _camera->m_front * _camera->m_near_far[1] - _camera->m_real_up * far_height));
    m_bottom = glm::vec4(bottom_normal, -glm::dot(bottom_normal, _camera->m_position));

    glm::vec3 top_normal = glm::normalize(glm::cross(
        _camera->m_front * _camera->m_near_far[1] + _camera->m_real_up * far_height,
        _camera->m_right));
    m_top = glm::vec4(top_normal, -glm::dot(top_normal, _camera->m_position));

    glm::vec3 near_normal = _camera->m_front;
    glm::vec3 near_point = _camera->m_position + _camera->m_front * _camera->m_near_far[0];
    m_near = glm::vec4(near_normal, -glm::dot(near_normal, near_point));

    glm::vec3 far_normal = -_camera->m_front;
    glm::vec3 far_point = _camera->m_position + _camera->m_front * _camera->m_near_far[1];
    m_far = glm::vec4(far_normal, -glm::dot(far_normal, far_point));
}

bool Camera::isVisible(const AABB<float> &_aabb) const {
    auto visible = [&_aabb](const glm::vec4 &plane) -> bool {
        return (glm::dot(plane, glm::vec4(_aabb.min.x, _aabb.min.y, _aabb.min.z, 1.f)) >= 0.f) ||
               (glm::dot(plane, glm::vec4(_aabb.max.x, _aabb.min.y, _aabb.min.z, 1.f)) >= 0.f) ||
               (glm::dot(plane, glm::vec4(_aabb.min.x, _aabb.max.y, _aabb.min.z, 1.f)) >= 0.f) ||
               (glm::dot(plane, glm::vec4(_aabb.max.x, _aabb.max.y, _aabb.min.z, 1.f)) >= 0.f) ||
               (glm::dot(plane, glm::vec4(_aabb.min.x, _aabb.min.y, _aabb.max.z, 1.f)) >= 0.f) ||
               (glm::dot(plane, glm::vec4(_aabb.max.x, _aabb.min.y, _aabb.max.z, 1.f)) >= 0.f) ||
               (glm::dot(plane, glm::vec4(_aabb.min.x, _aabb.max.y, _aabb.max.z, 1.f)) >= 0.f) ||
               (glm::dot(plane, glm::vec4(_aabb.max.x, _aabb.max.y, _aabb.max.z, 1.f)) >= 0.f);
    };

    return visible(m_frustum.m_near) &&
           visible(m_frustum.m_far) &&
           visible(m_frustum.m_top) &&
           visible(m_frustum.m_bottom) &&
           visible(m_frustum.m_right) &&
           visible(m_frustum.m_left);
}

void Camera::updateData() {
    m_orientation.x = glm::clamp(m_orientation.x, -M_PI_2_SAFE, M_PI_2_SAFE);
    m_orientation.y = Transformation::clipAnglePI(m_orientation.y);

    m_front = glm::normalize(Transformation::EulerToEuclidian(m_orientation));
    m_right = glm::normalize(glm::cross(m_front, VEC_UP));
    m_real_up = glm::normalize(glm::cross(m_right, m_front));

    m_projection = glm::perspective(m_fovy, m_aspect_ratio, m_near_far[0], m_near_far[1]);
    m_view = glm::lookAt(m_position, m_position + m_front, m_real_up);

    m_frustum.updatePlanes(this);
}

bool Camera::updateInterface(float _deltaTime) {
    bool disable_mouse_actions = false;
    if (ImGui::Begin("Camera Interface")) {
        disable_mouse_actions = ImGui::IsWindowHovered() || ImGui::IsAnyItemHovered() || ImGui::IsAnyItemActive() || ImGui::IsAnyItemFocused();

        // Camera Type Selection
        int current_type = static_cast<int>(m_type);
        if (ImGui::Combo("Camera Type", &current_type, CAMERA_TYPES)) {
            m_type = Type(current_type);
            switch (m_type) {
            case Type::Free:
                break;
            case Type::FirstPerson:
                break;
            case Type::Orbital:
                m_distance_to_center = glm::distance(m_position, *m_center);
                m_front = glm::normalize(*m_center - m_position);
                m_orientation = Transformation::EuclidianToEuler(m_front);
                break;
            }
            updateData();
        }

        ImGui::Separator();

        if (m_type == Type::Free || m_type == Type::FirstPerson) { // Free position Controls
            bool position_changed = ImGui::DragFloat3("Position", &m_position[0], 0.1f);
            if (position_changed) {
                updateData();
            }
            ImGui::Separator();
        }

        // Orientation Controls
        glm::vec2 angles_degree = glm::degrees(m_orientation);
        bool pitch_changed = ImGui::DragFloat("Pitch", &angles_degree[0], 1.f, -89.943f, 89.943f, "%.3f°");
        bool yaw_changed = ImGui::DragFloat("Yaw", &angles_degree[1], -1.f, -180.f, 180.f, "%.3f°");
        if (pitch_changed || yaw_changed) {
            m_orientation = glm::radians(angles_degree);
            updateData();
        }

        ImGui::Separator();

        // FOV Control
        float fovy_degree = glm::degrees(m_fovy);
        bool fovy_changed = ImGui::DragFloat("FOV", &fovy_degree, 0.1f, 1.f, 179.f, "%.3f°");
        if (fovy_changed) {
            m_fovy = glm::radians(fovy_degree);
        }

        ImGui::Separator();

        // Speed Controls
        if (m_type == Type::Free || m_type == Type::FirstPerson) {
            ImGui::DragFloat("Translation Speed", &m_translation_speed, 1.e-2f, 0.f, 1.e2f);
        } else {
            bool distance_changed = ImGui::DragFloat("Distance to Center", &m_distance_to_center, 0.1f, 1.e-4f, 1.e4f);
            if (distance_changed) {
                updateData();
            }
            ImGui::DragFloat("Zoom Rate", &m_zoom_rate, 1.e-4f, 0.f, 1.f);
        }
        ImGui::DragFloat("Rotation Speed", &m_rotation_speed, 1.e-4f, 0.f, 1.e2f);
    }

    ImGui::End();
    return disable_mouse_actions;
}

void Camera::updateKeyboardInput(GLFWwindow *_window, float _deltaTime) {
    static bool c_was_pressed = false;
    bool c_is_pressed = glfwGetKey(_window, GLFW_KEY_C) == GLFW_PRESS;
    if (c_is_pressed && !c_was_pressed) {
        m_type = Type((int(m_type) + 1) % CAMERA_TYPES_N);
        switch (m_type) {
        case Type::Free:
            break;
        case Type::FirstPerson:
            break;
        case Type::Orbital:
            m_distance_to_center = glm::distance(m_position, *m_center);
            m_front = glm::normalize(*m_center - m_position);
            m_orientation = Transformation::EuclidianToEuler(m_front);
            break;
        }
    }
    c_was_pressed = c_is_pressed;

    glm::vec3 motion = glm::vec3(
                           int(glfwGetKey(_window, GLFW_KEY_SPACE) == GLFW_PRESS) - int(glfwGetKey(_window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS),
                           int(glfwGetKey(_window, GLFW_KEY_D) == GLFW_PRESS) - int(glfwGetKey(_window, GLFW_KEY_A) == GLFW_PRESS),
                           int(glfwGetKey(_window, GLFW_KEY_W) == GLFW_PRESS) - int(glfwGetKey(_window, GLFW_KEY_S) == GLFW_PRESS)) *
                       _deltaTime * m_translation_speed;

    glm::vec3 flat_front = glm::cross(VEC_UP, m_right);
    switch (m_type) {
    case Type::Free:
        m_position += motion.x * m_real_up + motion.y * m_right + motion.z * m_front;
        break;
    case Type::FirstPerson:
        m_position += motion.x * VEC_UP + motion.y * m_right + motion.z * flat_front;
        break;
    case Type::Orbital:
        m_distance_to_center = glm::distance(m_position, *m_center);
        m_front = glm::normalize(*m_center - m_position);
        m_orientation = Transformation::EuclidianToEuler(m_front);
        break;
    }
}

void Camera::updateMouseInput(GLFWwindow *_window, float _deltaTime, const glm::vec2 &_cursor_vel, const glm::vec2 &_scroll, bool _disable_actions) {
    float rotation_speed = _deltaTime * m_rotation_speed;
    switch (m_type) {
    case Type::Free:
    case Type::FirstPerson:
        if (!_disable_actions && glfwGetMouseButton(_window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
            m_orientation.x -= rotation_speed * _cursor_vel.y;
            m_orientation.y -= rotation_speed * _cursor_vel.x;
        }
        break;
    case Type::Orbital:
        if (!_disable_actions) {
            m_distance_to_center = glm::max(m_distance_to_center * (1.f - _scroll.y * m_zoom_rate), 1.e-4f);
            if (glfwGetMouseButton(_window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
                m_orientation.x -= rotation_speed * _cursor_vel.y;
                m_orientation.y -= rotation_speed * _cursor_vel.x;
                updateData();
                m_position = *m_center - m_distance_to_center * m_front;
                break;
            }
        }

        // update target pos
        m_position = *m_center - m_distance_to_center * m_front;

        // re update angle
        m_front = *m_center - m_position;
        m_orientation = Transformation::EuclidianToEuler(m_front);
        break;
    }
}

void Camera::update(GLFWwindow *_window, float _deltaTime, const glm::vec2 &_cursor_vel, const glm::vec2 &_scroll) {
    bool disable_mouse_actions = updateInterface(_deltaTime);

    int window_width, window_height;
    glfwGetWindowSize(_window, &window_width, &window_height);
    m_aspect_ratio = float(window_width) / window_height;

    updateKeyboardInput(_window, _deltaTime);
    updateMouseInput(_window, _deltaTime, _cursor_vel, _scroll, disable_mouse_actions);

    updateData();
}
