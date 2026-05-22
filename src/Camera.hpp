#pragma once

#include <glm/glm.hpp>

#include "ECS/ECS.hpp"

class Camera {
public:
    struct Frustum {
    private:
        glm::vec4 m_left;
        glm::vec4 m_right;

        glm::vec4 m_bottom;
        glm::vec4 m_top;

        glm::vec4 m_near;
        glm::vec4 m_far;

        constexpr bool isVisible(const glm::vec4& _plane, float x, float y, float z) const {
            return _plane.x * x + _plane.y * y + _plane.z * z + _plane.w >= 0;
        }
        constexpr bool isVisible(const glm::vec4& _plane, const glm::vec3& _pos) const {
            return _plane.x * _pos.x + _plane.y * _pos.y + _plane.z * _pos.z + _plane.w >= 0;
        }
        constexpr bool isVisible(const glm::vec4& plane, const AABB<float>& aabb) const {
            return isVisible(plane,
                             plane.x >= 0 ? aabb.max.x : aabb.min.x,
                             plane.y >= 0 ? aabb.max.y : aabb.min.y,
                             plane.z >= 0 ? aabb.max.z : aabb.min.z);
        }

    public:
        Frustum() {};

        inline void updatePlanes(const glm::mat4& view_projection_transpose) {
            m_left = view_projection_transpose[3] + view_projection_transpose[0];
            m_right = view_projection_transpose[3] - view_projection_transpose[0];
            m_bottom = view_projection_transpose[3] + view_projection_transpose[1];
            m_top = view_projection_transpose[3] - view_projection_transpose[1];
            m_near = view_projection_transpose[3] + view_projection_transpose[2];
            m_far = view_projection_transpose[3] - view_projection_transpose[2];
        }
        inline void updatePlanes(const glm::mat4& projection, const glm::mat4& view) {
            updatePlanes(glm::transpose(projection * view));
        }

        inline bool isVisible(float x, float y, float z) const {
            return !(!isVisible(m_near, x, y, z) ||
                     !isVisible(m_far, x, y, z) ||
                     !isVisible(m_top, x, y, z) ||
                     !isVisible(m_bottom, x, y, z) ||
                     !isVisible(m_right, x, y, z) ||
                     !isVisible(m_left, x, y, z));
        }
        inline bool isVisible(const glm::vec3& _pos) const {
            return !(!isVisible(m_near, _pos) ||
                     !isVisible(m_far, _pos) ||
                     !isVisible(m_top, _pos) ||
                     !isVisible(m_bottom, _pos) ||
                     !isVisible(m_right, _pos) ||
                     !isVisible(m_left, _pos));
        }

        bool isVisible(const AABB<float>& aabb) const {
            return !(!isVisible(m_near, aabb) ||
                     !isVisible(m_far, aabb) ||
                     !isVisible(m_top, aabb) ||
                     !isVisible(m_bottom, aabb) ||
                     !isVisible(m_right, aabb) ||
                     !isVisible(m_left, aabb));
        }
    };

private:
    bool m_disable_mouse_actions{false};

    glm::vec2 m_cam_orientation{0.};
    glm::vec3 m_front{0.};
    glm::vec3 m_right{0.};
    glm::vec3 m_real_up{0.};
    glm::mat4 m_view{0.};
    glm::mat4 m_projection{0.};
    Frustum m_frustum;

public:
    glm::vec3 m_cam_pos;
    float m_free_cam_speed = 16.f;
    float m_fovy{M_PI_2f};
    glm::vec2 m_near_far{1.e-1f, 1.e8f};

private:
    inline void applyPosConstraint(ECS::Positionnable& positionnable, ECS::Controllable& controllable) {
        switch (controllable.type) {
        case ECS::ControlType::FreeCam:
            break;
        case ECS::ControlType::FirstPerson:
            m_cam_pos = positionnable.pos + controllable.eye_pos;
            break;
        case ECS::ControlType::ThirdPerson:
            // update target pos
            m_cam_pos = positionnable.pos + controllable.eye_pos - controllable.distance_to_center * m_front;

            // re update angle
            m_front = positionnable.pos + controllable.eye_pos - m_cam_pos;
            m_cam_orientation = Transformation::EuclidianToEuler(m_front);
            Transformation::getViewVectors(m_cam_orientation, m_front, m_right, m_real_up);
            break;
        case ECS::ControlType::__COUNT:
            break;
        }
    }
    inline void updateRenderingData(float _aspect_ratio) {
        m_projection = glm::perspective(m_fovy, _aspect_ratio, m_near_far[0], m_near_far[1]);
        m_view = glm::lookAt(m_cam_pos, m_cam_pos + m_front, m_real_up);
        m_frustum.updatePlanes(m_projection, m_view);
    }
    inline void updateFreeFrustum(float _aspect_ratio, const glm::vec3& pos, const glm::vec3& front, const glm::vec3& real_up) {
        glm::mat4 projection = glm::perspective(m_fovy, _aspect_ratio, m_near_far[0], m_near_far[1]);
        glm::mat4 view = glm::lookAt(pos, pos + front, real_up);
        m_frustum.updatePlanes(projection, view);
    }

    inline void updateFreeKeyboardInput(Window& _window, float _deltaTime) {
        glm::vec3 motion = glm::vec3(
            _window.keyboard.isHeld(GLFW_KEY_SPACE) - _window.keyboard.isHeld(GLFW_KEY_LEFT_CONTROL),
            _window.keyboard.isHeld(GLFW_KEY_D) - _window.keyboard.isHeld(GLFW_KEY_A),
            _window.keyboard.isHeld(GLFW_KEY_W) - _window.keyboard.isHeld(GLFW_KEY_S));
        if (motion.x != 0.f || motion.y != 0.f || motion.z != 0.f)
            motion = glm::normalize(motion);
        glm::vec3 flat_front = glm::cross(MathHelpers::VEC_UP, m_right);
        m_cam_pos += _deltaTime * m_free_cam_speed * (motion.x * MathHelpers::VEC_UP + motion.y * m_right + motion.z * flat_front);
    }

    inline void updateEntityKeyboardInput(ECS::Movable& movable, ECS::Groundable& groundable, ECS::Orientable& orientable, Window& _window, float _deltaTime) {
        glm::vec2 motion = glm::vec2(
            _window.keyboard.isHeld(GLFW_KEY_D) - _window.keyboard.isHeld(GLFW_KEY_A),
            _window.keyboard.isHeld(GLFW_KEY_W) - _window.keyboard.isHeld(GLFW_KEY_S));
        if (motion.x != 0.f || motion.y != 0.f)
            motion = glm::normalize(motion);
        glm::vec3 front = Transformation::EulerToEuclidian(orientable.orientation);
        glm::vec3 right = glm::normalize(glm::cross(front, MathHelpers::VEC_UP));
        glm::vec3 flat_front = glm::cross(MathHelpers::VEC_UP, right);

        movable.vel += (groundable.on_ground ? groundable.walk_speed : groundable.air_control_speed) * (motion.x * right + motion.y * flat_front);
        if (_window.keyboard.isHeld(GLFW_KEY_SPACE) && groundable.on_ground) {
            movable.vel += MathHelpers::VEC_UP * groundable.jump_force;
            groundable.on_ground = false;
        }
    }

    inline void updateMouseInput(Window& _window, float _deltaTime) {
        float rotation_speed = _deltaTime * _window.m_rotation_speed;
        if (glfwGetMouseButton(_window.getWindow(), GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
            m_cam_orientation.x -= rotation_speed * _window.getCursorVel().y;
            m_cam_orientation.y -= rotation_speed * _window.getCursorVel().x;
            Transformation::clampOrientation(m_cam_orientation);
            Transformation::getViewVectors(m_cam_orientation, m_front, m_right, m_real_up);
        }
    }

public:
    Camera() {
        Transformation::getViewVectors(m_cam_orientation, m_front, m_right, m_real_up);
        updateRenderingData(16.f / 9.f);
    }

    inline const glm::vec3& getCamPos() const { return m_cam_pos; }
    inline const glm::mat4& getView() const { return m_view; }
    inline const glm::mat4& getProjection() const { return m_projection; }
    inline const Frustum& getFrustum() const { return m_frustum; }

    void update(Window& _window, ECSManager& _ecs_manager, float _deltaTime) {
        ECS::ControllingSystem controlling = _ecs_manager.getSystem<ECS::ControllingSystem>();
        bool has_entity = controlling.getControlledEntity().has_value();

        if (!m_disable_mouse_actions) {
            updateMouseInput(_window, _deltaTime);
        }

        if (!has_entity) {
            updateFreeKeyboardInput(_window, _deltaTime);
            updateRenderingData(_window.getAspectRatio());
            return;
        }

        ECS::EntityId entity = controlling.getControlledEntity().value();
        ECS::Positionnable& positionnable = _ecs_manager.getComponent<ECS::Positionnable>(entity);
        ECS::Orientable& orientable = _ecs_manager.getComponent<ECS::Orientable>(entity);
        ECS::Controllable& controllable = _ecs_manager.getComponent<ECS::Controllable>(entity);
        ECS::Movable& movable = _ecs_manager.getComponent<ECS::Movable>(entity);
        ECS::Groundable& groundable = _ecs_manager.getComponent<ECS::Groundable>(entity);
        if (controllable.type != ECS::ControlType::FreeCam) {
            orientable.orientation = m_cam_orientation;
            updateEntityKeyboardInput(movable, groundable, orientable, _window, _deltaTime);
            applyPosConstraint(positionnable, controllable);
            updateRenderingData(_window.getAspectRatio());
        } else {
            updateFreeKeyboardInput(_window, _deltaTime);
            glm::vec3 front, right, real_up;
            Transformation::getViewVectors(orientable.orientation, front, right, real_up);
            updateFreeFrustum(_window.getAspectRatio(), positionnable.pos, front, real_up);
        }
    }

    // void updateInterface(ECS::ComponentManager& cm, Window& _window) {
    //     m_disable_mouse_actions = false;
    //     if (ImGui::Begin("Camera Interface")) {
    //         m_disable_mouse_actions = ImGui::IsWindowHovered() || ImGui::IsAnyItemHovered() || ImGui::IsAnyItemActive() || ImGui::IsAnyItemFocused();

    //         // Camera Type Selection
    //         int current_type = static_cast<int>(m_control_type);
    //         if (ImGui::Combo("Camera Type", &current_type, CONTROL_TYPES_STR)) {
    //             changeControlType(cm, ControlType(current_type));
    //         }

    //         ImGui::Separator();
    //         ImGui::BeginDisabled(m_control_type == ControlType::FreeCam);
    //         ImGui::DragFloat3("Position", &m_cam_pos[0], 0.1f);
    //         ImGui::Separator();
    //         ImGui::EndDisabled();

    //         // Orientation Controls
    //         glm::vec2 angles_degree = glm::degrees(m_cam_orientation);
    //         bool pitch_changed = ImGui::DragFloat("Pitch", &angles_degree[0], 1.f, -89.943f, 89.943f, "%.3f°");
    //         bool yaw_changed = ImGui::DragFloat("Yaw", &angles_degree[1], -1.f, -180.f, 180.f, "%.3f°");
    //         if (pitch_changed || yaw_changed) {
    //             m_cam_orientation = glm::radians(angles_degree);
    //             Transformation::clampOrientation(m_cam_orientation);
    //             Transformation::getViewVectors(m_cam_orientation, m_front, m_right, m_real_up);
    //         }

    //         ImGui::Separator();

    //         // FOV Control
    //         float fovy_degree = glm::degrees(m_fovy);
    //         bool fovy_changed = ImGui::DragFloat("FOV", &fovy_degree, 0.1f, 1.f, 179.f, "%.3f°");
    //         if (fovy_changed) {
    //             m_fovy = glm::radians(fovy_degree);
    //         }

    //         ImGui::Separator();

    //         // Speed Controls
    //         ImGui::DragFloat("Rotation Speed", &_window.m_rotation_speed, 1.e-4f, 0.f, 1.e2f);
    //         ImGui::BeginDisabled(m_control_type != ControlType::FreeCam);
    //         ImGui::DragFloat("Translation Speed", &m_free_cam_speed, 1.e-2f, 0.f, 1.e2f);
    //         ImGui::EndDisabled();

    //         // Distance to center
    //         if (m_controlled_entity.has_value()) {
    //             ImGui::BeginDisabled(m_control_type != ControlType::ThirdPerson);
    //             ImGui::DragFloat("Distance to Center", &cm.getComponent<ECS::Controllable>(m_controlled_entity.value()).distance_to_center, 0.1f, 1.e-4f, 1.e4f);
    //             ImGui::DragFloat("Zoom Rate", &_window.m_zoom_rate, 1.e-4f, 0.f, 1.f);
    //             ImGui::EndDisabled();
    //         }
    //     }

    //     ImGui::End();
    // }
};