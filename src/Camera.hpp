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
    float m_fovy{M_PI_2f};
    glm::vec2 m_near_far{1.e-1f, 1.e8f};
    float m_rotation_speed{0.5f};
    float m_free_cam_speed = 16.f;
    float m_zoom_rate{0.05f};
    float m_elasticity{0.5f};
    bool m_update_frustum{true};

private:
    inline void applyPosConstraint(const World* _world, const ECS::Positionnable& positionnable, const ECS::Orientable& orientable, const ECS::Controllable& controllable) {
        glm::vec3 should_pos;
        glm::vec3 eye_pos = positionnable.pos + orientable.eye_pos;
        uint i = 0;
        std::vector<const Block*> blocks;
        switch (controllable.type) {
        case ECS::ControlType::FreeCam:
            break;
        case ECS::ControlType::FirstPerson:
            // En FPS pas de mouvement elastique sinon préparer sac à vomi
            m_cam_pos = eye_pos;
            break;
        case ECS::ControlType::ThirdPerson:
            // update target pos
            should_pos = eye_pos - controllable.distance_to_center * m_front;
            m_cam_pos = (1.f - m_elasticity) * m_cam_pos + m_elasticity * should_pos;

            blocks = _world->findBlockLine(eye_pos, m_cam_pos);
            while (i < blocks.size() && (blocks[i] == nullptr || !blocks[i]->hasHitbox()))
                i++;
            if (i < blocks.size() && blocks[i] != nullptr) {
                glm::ivec3 block_pos = blocks[i]->getPos();
                AABB<float> block_aabb(block_pos, block_pos + glm::ivec3(1));
                float tmin, tmax;
                uint face_min, face_max;
                if (block_aabb.intersectRay(eye_pos, m_cam_pos - eye_pos, tmin, tmax, face_min, face_max)) {
                    m_cam_pos = eye_pos + tmin * (m_cam_pos - eye_pos);
                }
            }

            // re update angle
            m_front = eye_pos - should_pos;
            m_cam_orientation = Transformation::EuclidianToEuler(m_front);
            Transformation::getViewVectors(m_cam_orientation, m_front, m_right, m_real_up);
            break;
        case ECS::ControlType::__COUNT:
            break;
        }
    }

    inline void toggleMouseAction() { m_disable_mouse_actions = !m_disable_mouse_actions; }

    inline void updateRenderingData(float _aspect_ratio) {
        m_projection = glm::perspective(m_fovy, _aspect_ratio, m_near_far[0], m_near_far[1]);
        m_view = glm::lookAt(m_cam_pos, m_cam_pos + m_front, m_real_up);
        if (m_update_frustum)
            m_frustum.updatePlanes(m_projection, m_view);
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

    inline void updateMouseInput(Window& _window, float _deltaTime) {
        float rotation_speed = _deltaTime * m_rotation_speed;
        if (_window.getMouseCapture()) {
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

    void update(Window& _window, const World* _world, ECSManager& _ecs_manager, float _deltaTime) {
        ECS::ControllingSystem& controlling = _ecs_manager.getSystem<ECS::ControllingSystem>();
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
        const ECS::Positionnable& positionnable = _ecs_manager.getComponent<ECS::Positionnable>(entity);
        ECS::Orientable& orientable = _ecs_manager.getComponent<ECS::Orientable>(entity);
        const ECS::Controllable& controllable = _ecs_manager.getComponent<ECS::Controllable>(entity);
        if (controllable.type == ECS::ControlType::FreeCam) {
            updateFreeKeyboardInput(_window, _deltaTime);
            updateRenderingData(_window.getAspectRatio());
        } else {
            orientable.orientation = m_cam_orientation;
            applyPosConstraint(_world, positionnable, orientable, controllable);
            updateRenderingData(_window.getAspectRatio());
        }
    }

    inline const glm::vec3 getFront() const { return m_front; }

    void updateInterface(ECSManager& _ecs_manager) {
        m_disable_mouse_actions = false;
        ECS::ControlType fallback_type{ECS::ControlType::FreeCam};
        float fallback_distance{0.f};
        glm::vec3 fallback_pos{0.f};

        ECS::ControllingSystem& controlling = _ecs_manager.getSystem<ECS::ControllingSystem>();
        bool has_entity = controlling.getControlledEntity().has_value();
        ECS::ControlType& control_type = has_entity ? _ecs_manager.getComponent<ECS::Controllable>(controlling.getControlledEntity().value()).type
                                                    : fallback_type;
        float& distance_to_center = has_entity ? _ecs_manager.getComponent<ECS::Controllable>(controlling.getControlledEntity().value()).distance_to_center
                                               : fallback_distance;
        glm::vec3& entity_pos = has_entity ? _ecs_manager.getComponent<ECS::Positionnable>(controlling.getControlledEntity().value()).pos
                                           : fallback_pos;

        if (!ImGui::Begin("Camera Interface")) {
            ImGui::End();
            return;
        }
        m_disable_mouse_actions = ImGui::IsWindowHovered() || ImGui::IsAnyItemHovered() || ImGui::IsAnyItemActive() || ImGui::IsAnyItemFocused();

        // Camera Type Selection
        ImGui::BeginDisabled(!has_entity);
        int control_type_int = static_cast<int>(control_type);
        if (ImGui::Combo("Camera Type", &control_type_int, ECS::CONTROL_TYPES_STR)) {
            control_type = static_cast<ECS::ControlType>(control_type_int);
        }
        ImGui::EndDisabled();

        ImGui::Separator();

        ImGui::BeginDisabled(control_type != ECS::ControlType::FreeCam);
        ImGui::DragFloat3("Position", &m_cam_pos[0], 0.1f);
        ImGui::EndDisabled();

        ImGui::BeginDisabled(control_type != ECS::ControlType::FreeCam);
        if (ImGui::Button("Snap entity to Pos")) {
            entity_pos = m_cam_pos;
        }
        ImGui::EndDisabled();
        ImGui::SameLine();
        ImGui::Checkbox("update frustum", &m_update_frustum);
        ImGui::Separator();

        // Orientation Controls
        glm::vec2 angles_degree = glm::degrees(m_cam_orientation);
        bool pitch_changed = ImGui::DragFloat("Pitch", &angles_degree[0], .5f, -89.943f, 89.943f, "%.3f°");
        bool yaw_changed = ImGui::DragFloat("Yaw", &angles_degree[1], -1.f, -180.f, 180.f, "%.3f°");
        if (pitch_changed || yaw_changed) {
            m_cam_orientation = glm::radians(angles_degree);
            Transformation::clampOrientation(m_cam_orientation);
            Transformation::getViewVectors(m_cam_orientation, m_front, m_right, m_real_up);
        }

        ImGui::Separator();

        // FOV Control
        float fovy_degree = glm::degrees(m_fovy);
        bool fovy_changed = ImGui::DragFloat("FOV", &fovy_degree, 0.1f, 1.f, 179.f, "%.3f°");
        if (fovy_changed) {
            m_fovy = glm::radians(fovy_degree);
        }
        ImGui::DragFloat2("nearFar", glm::value_ptr(m_near_far), 1.e-4f, 0.f, 1.e8f);
        ImGui::DragFloat("rotationSpeed", &m_rotation_speed, 1.e-2f, 0.f, 1.e2f);

        ImGui::Separator();

        ImGui::BeginDisabled(control_type != ECS::ControlType::FreeCam);
        ImGui::DragFloat("freeCamSpeed", &m_free_cam_speed, 1.f, 0.f, 1.e8f);
        ImGui::EndDisabled();

        // Distance to center
        ImGui::BeginDisabled(control_type != ECS::ControlType::ThirdPerson);
        ImGui::DragFloat("Distance to Center", &distance_to_center, 0.1f, 1.e-4f, 1.e4f);
        ImGui::DragFloat("Zoom Rate", &m_zoom_rate, 1.e-4f, 0.f, 1.f);
        ImGui::DragFloat("elasticity", &m_elasticity, 1.e-4f, 0.f, 1.f);
        ImGui::EndDisabled();

        ImGui::End();
    }
};