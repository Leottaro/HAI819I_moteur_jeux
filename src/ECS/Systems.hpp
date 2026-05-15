#pragma once
#include "Data.hpp"

namespace WORLD_API {
bool isChunkLoaded(ECS::Positionnable& pos);
Block* findBlock(ECS::Positionnable& pos, const glm::ivec3& _block_pos);
} // namespace WORLD_API

// -------------------------------------------------------------------------
// SYSTEMS
// -------------------------------------------------------------------------

template <ECS::Component... Cs>
struct SystemBase {
    // Built entirely at compile time from the component pack
    static constexpr ECS::ComponentSignature signature = []() {
        ECS::ComponentSignature sig{};
        (sig.set(ECS::component_id<Cs>), ...);
        return sig;
    }();

    std::unordered_set<ECS::EntityId> m_entities{};
};

class ECS::PositionSystem : public SystemBase<ECS::Positionnable> {
public:
    inline void init(ComponentManager& cm, ECS::EntityId entity) {}
    inline void clear(ComponentManager& cm, ECS::EntityId entity) {}

    std::vector<ECS::EntityId> getOutOfBoundEntities(ComponentManager& cm) const {
        std::vector<ECS::EntityId> entities;
        for (ECS::EntityId entity : m_entities) {
            ECS::Positionnable& positionnable = cm.getComponent<ECS::Positionnable>(entity);
            if (!WORLD_API::isChunkLoaded(positionnable))
                entities.push_back(entity);
        }

        return entities;
    }
};

class ECS::PhysicsSystem : public SystemBase<ECS::Positionnable, ECS::Movable, ECS::Collisionnable, ECS::Groundable, ECS::PhysicsStats> {
public:
    inline void init(ComponentManager& cm, ECS::EntityId entity) {}
    inline void clear(ComponentManager& cm, ECS::EntityId entity) {}

    void update(ComponentManager& cm, float _deltaTime) {
        for (ECS::EntityId entity : m_entities) {
            ECS::Positionnable& positionnable = cm.getComponent<ECS::Positionnable>(entity);
            ECS::Movable& movable = cm.getComponent<ECS::Movable>(entity);
            ECS::Collisionnable& collisionable = cm.getComponent<ECS::Collisionnable>(entity);
            ECS::Groundable& groundable = cm.getComponent<ECS::Groundable>(entity);
            ECS::PhysicsStats& stats = cm.getComponent<ECS::PhysicsStats>(entity);

            glm::vec3 acceleration{0.f};
            if (groundable.on_ground) {
                groundable.on_ground = false;
                float max_static_friction = 0.f;
                for (const AABB<float>& hitbox : collisionable.hitboxes) {
                    glm::ivec3 min_block = Block::posToBlockPos(positionnable.pos + hitbox.min);
                    glm::ivec3 max_block = Block::posToBlockPos(positionnable.pos + hitbox.max);
                    glm::ivec3 under_block_pos{0, min_block.y - 1, 0};
                    for (under_block_pos.z = min_block.z; under_block_pos.z <= max_block.z; under_block_pos.z++) {
                        for (under_block_pos.x = min_block.x; under_block_pos.x <= max_block.x; under_block_pos.x++) {
                            Block* block = WORLD_API::findBlock(positionnable, under_block_pos);
                            groundable.on_ground = groundable.on_ground || (block != nullptr && block->getType() != BlockType::Air);
                            max_static_friction = block != nullptr ? std::max(max_static_friction, block->getStaticFriction()) : max_static_friction;
                        }
                    }
                }
                if (groundable.on_ground) {
                    movable.vel.x *= 1.f - max_static_friction;
                    movable.vel.y = 0.f;
                    movable.vel.z *= 1.f - max_static_friction;
                }
            } else {
                glm::vec3 gravity = G * stats.weight; // g
                acceleration += gravity;
                float densite_fluide = WORLD_API::findBlock(positionnable, positionnable.pos)->getDensity();
                if (densite_fluide > 0.f) {
                    acceleration += densite_fluide * -gravity * stats.volume / (stats.weight / stats.volume); // flottaison
                    acceleration += densite_fluide * movable.vel * -stats.drag;                               // drag
                }
            }

            movable.vel += _deltaTime * acceleration / stats.weight;
        }
    }
};

class ECS::WorldCollisionSystem : public SystemBase<ECS::Positionnable, ECS::Collisionnable, ECS::Movable, ECS::Groundable> {
    inline void bounce(float _static_friction, float _friction, float _restitution, const glm::vec3& _normal, glm::vec3& _vel) {
        float v_dot_n = glm::dot(_vel, _normal);
        glm::vec3 v_normal = v_dot_n * _normal;
        glm::vec3 v_tangent = _vel - v_normal;
        v_tangent *= 1.f - _friction;
        if (glm::length(v_tangent) < _static_friction * glm::length(v_normal)) {
            v_tangent *= 0.f;
        }
        v_normal *= -_restitution;
        if (glm::length(v_normal) < 0.05f) {
            v_normal = glm::vec3(0.f);
        }
        _vel = v_normal + v_tangent;
    }
    struct CollisionsInfos {
        float t{std::numeric_limits<float>::max()};
        glm::vec3 normal{0.f};
        glm::vec3 pos{0.f};
        float friction{0.f};
        float restitution{1.f};
    };
    // Return true if it detected a collision
    bool detectCollision(float _deltaTime, CollisionsInfos& res, ECS::Positionnable& _positionnable, ECS::Collisionnable& _collisionnable, ECS::Movable& _movable) {
        res.t = std::numeric_limits<float>::max();

        std::vector<Block*> to_explore;
        for (const AABB<float>& hitbox : _collisionnable.hitboxes) {
            glm::ivec3 min_block = Block::posToBlockPos(_positionnable.pos + hitbox.min);
            glm::ivec3 max_block = Block::posToBlockPos(_positionnable.pos + hitbox.max);
            for (uint y : {min_block.y - 1, min_block.y, max_block.y, max_block.y + 1})
                for (uint z : {min_block.z - 1, min_block.z, max_block.z, max_block.z + 1})
                    for (uint x : {min_block.x - 1, min_block.x, max_block.x, max_block.x + 1}) {
                        Block* block = WORLD_API::findBlock(_positionnable, glm::ivec3(x, y, z));
                        if (block != nullptr)
                            to_explore.push_back(block);
                    }
        }

        std::set<Block*> explored;
        while (!to_explore.empty()) {
            Block* block = to_explore.back();
            to_explore.pop_back();
            if (!explored.insert(block).second)
                continue;

            glm::ivec3 block_pos = block->getPos();
            AABB<float> block_aabb(block_pos, block_pos + glm::ivec3(1));
            // std::cout << "\t\t" << size_t(block->getType()) << " at " << glm::to_string(block_pos) << std::endl;

            bool clip = false;
            for (const AABB<float>& hitbox : _collisionnable.hitboxes) {
                glm::vec3 dist;
                if (block_aabb.intersectAABB(_positionnable.pos + hitbox, dist)) {
                    clip = true;
                    continue;
                }

                float t;
                glm::vec3 normal(0.);
                if (block->hasHitbox() && block_aabb.intersectAABB(_positionnable.pos + hitbox, _deltaTime * _movable.vel, t, normal)) {
                    if (t < res.t) {
                        res.t = t;
                        res.normal = normal;
                        res.pos = _positionnable.pos + t * _deltaTime * _movable.vel;
                        while (block_aabb.intersectAABB(res.pos + hitbox, dist)) {
                            res.pos += dist;
                        }

                        glm::vec3 block_center = glm::vec3(block->getPos()) + glm::vec3(0.5f);
                        glm::ivec3 min_collision_block = Block::posToBlockPos(MathHelpers::projectPointOnPlane(res.pos + hitbox.min, block_center, normal));
                        glm::ivec3 max_collision_block = Block::posToBlockPos(MathHelpers::projectPointOnPlane(res.pos + hitbox.max, block_center, normal));

                        glm::ivec3 collision_block;
                        res.friction = 0.f;
                        res.restitution = 1.f;
                        for (collision_block.y = min_collision_block.y; collision_block.y <= max_collision_block.y; collision_block.y++) {
                            for (collision_block.z = min_collision_block.z; collision_block.z <= max_collision_block.z; collision_block.z++) {
                                for (collision_block.x = min_collision_block.x; collision_block.x <= max_collision_block.x; collision_block.x++) {
                                    Block* block = WORLD_API::findBlock(_positionnable, collision_block);
                                    if (block == nullptr || block->getType() == BlockType::Air)
                                        continue;
                                    res.friction = std::max(res.friction, block->getFriction());
                                    res.restitution = std::min(res.restitution, block->getRestitution());
                                }
                            }
                        }
                    }
                    // std::cout << "\t\t\tintersection: " << "t=" << res.t << "\tnormal=" << glm::to_string(res.normal) << "\tpos=" << glm::to_string(res.pos) << std::endl;
                }
            }

            if (clip) {
                // std::cout << "\t\tclip" << std::endl;
                for (int face_i = 0; face_i < 6; face_i++)
                    if (block->m_neighbours[face_i] != nullptr && explored.find(block->m_neighbours[face_i]) == explored.end())
                        to_explore.push_back(block->m_neighbours[face_i]);
            }
        }

        return res.t <= 1.f;
    }

    void updateEntity(ComponentManager& cm, float _deltaTime, ECS::EntityId _entity) {
        ECS::Positionnable& positionnable = cm.getComponent<ECS::Positionnable>(_entity);
        ECS::Collisionnable& collisionable = cm.getComponent<ECS::Collisionnable>(_entity);
        ECS::Movable& movable = cm.getComponent<ECS::Movable>(_entity);
        ECS::Groundable& groundable = cm.getComponent<ECS::Groundable>(_entity);

        // Chunk collision detection
        CollisionsInfos collision;
        // std::cout << std::endl
        //           << std::endl
        //           << std::endl
        //           << "Starting detection: dt=" << _deltaTime << std::endl;
        while (detectCollision(_deltaTime, collision, positionnable, collisionable, movable)) {
            // std::cout << "\tCOLLISION: " << std::endl
            //           << "\t\t" << size_t(collision.block->getType()) << " at " << glm::to_string(collision.block->getPos()) << std::endl
            //           << "\t\tt: " << collision.t << std::endl
            //           << "\t\tnormal: " << glm::to_string(collision.normal) << std::endl
            //           << "\t\told pos: " << glm::to_string(positionnable.pos) << std::endl
            //           << "\t\told vel: " << glm::to_string(movable.vel) << std::endl;

            bounce(0.f, collision.friction, collision.restitution, collision.normal, movable.vel);
            if (collision.normal == glm::vec3(0.f, 1.f, 0.f) && movable.vel.y <= -0.05f * ECS::G.y) {
                groundable.on_ground = true;
                collision.pos.y += 1.e-2f;
            }
            positionnable.pos = collision.pos;
            if (!WORLD_API::isChunkLoaded(positionnable)) {
                return;
            }
            _deltaTime *= (1.f - collision.t);
            // std::cout
            //     << "\t\tpos: " << glm::to_string(positionnable.pos) << std::endl
            //     << "\t\tvel: " << glm::to_string(movable.vel) << std::endl
            //     << "dt=" << _deltaTime << std::endl;
        }
        positionnable.pos += _deltaTime * movable.vel;
        // std::cout << "\tAPPLIED dt=" << _deltaTime << " velocity" << std::endl
        //           << "\t\tnew pos: " << glm::to_string(positionnable.pos) << std::endl
        //           << "\t\tnew vel: " << glm::to_string(movable.vel) << std::endl;

        if (!WORLD_API::isChunkLoaded(positionnable)) {
            return;
        }

        return;
    }

public:
    inline void init(ComponentManager& cm, ECS::EntityId entity) {}
    inline void clear(ComponentManager& cm, ECS::EntityId entity) {}

    inline void update(ComponentManager& cm, float _deltaTime) {
        for (ECS::EntityId entity : m_entities)
            updateEntity(cm, _deltaTime, entity);
    }
};

class ECS::HitBoxDisplaySystem : public SystemBase<ECS::Positionnable, ECS::Collisionnable, ECS::CollisionDisplay> {
public:
    inline void init(ComponentManager& cm, ECS::EntityId entity) {
        ECS::Collisionnable& collisionnable = cm.getComponent<ECS::Collisionnable>(entity);
        ECS::CollisionDisplay& collision_display = cm.getComponent<ECS::CollisionDisplay>(entity);
        collision_display.boxes.resize(collisionnable.hitboxes.size());
        for (size_t i = 0; i < collisionnable.hitboxes.size(); i++) {
            collision_display.boxes[i].initShaderData(collisionnable.hitboxes[i]);
        }
    }

    inline void render(const ComponentManager& cm, ShaderProgram& _shader) const {
        _shader.set("color", glm::vec3(1.f));
        for (ECS::EntityId entity : m_entities) {
            const ECS::Positionnable& positionnable = cm.getComponent<ECS::Positionnable>(entity);
            const ECS::CollisionDisplay& collision_display = cm.getComponent<ECS::CollisionDisplay>(entity);
            _shader.set("position", positionnable.pos);
            for (const AABBRenderer& box : collision_display.boxes)
                box.render();
        }
    }

    inline void clear(ComponentManager& cm, ECS::EntityId entity) {
        ECS::CollisionDisplay& collision_display = cm.getComponent<ECS::CollisionDisplay>(entity);
        for (AABBRenderer& box : collision_display.boxes)
            box.clearShaderData();
    }
};

class ECS::CamerableSystem : public SystemBase<ECS::Positionnable, ECS::Orientable, ECS::Camerable> {
    ECS::ControlType m_control_type{ECS::ControlType::ThirdPerson};
    std::optional<ECS::EntityId> m_controlled_entity{};

    glm::vec3 m_cam_pos;
    glm::vec2 m_cam_orientation;
    glm::vec3 m_front;
    glm::vec3 m_right;
    glm::vec3 m_real_up;
    glm::mat4 m_view;
    glm::mat4 m_projection;
    Frustum m_frustum;

    inline void applyPosConstraint(ECS::Positionnable& positionnable, ECS::Camerable& camerable) {
        switch (m_control_type) {
        case ControlType::FreeCam:
            break;
        case ControlType::FirstPerson:
            m_cam_pos = positionnable.pos + camerable.eye_pos;
            break;
        case ControlType::ThirdPerson:
            // update target pos
            m_cam_pos = positionnable.pos + camerable.eye_pos - camerable.distance_to_center * m_front;

            // re update angle
            m_front = positionnable.pos + camerable.eye_pos - m_cam_pos;
            m_cam_orientation = Transformation::EuclidianToEuler(m_front);
            Transformation::getViewVectors(m_cam_orientation, m_front, m_right, m_real_up);
            break;
        case ControlType::__COUNT:
            break;
        }
    }
    inline void updateRenderingData(float _aspect_ratio) {
        m_projection = glm::perspective(m_fovy, _aspect_ratio, m_near_far[0], m_near_far[1]);
        m_view = glm::lookAt(m_cam_pos, m_cam_pos + m_front, m_real_up);
        m_frustum.updatePlanes(m_projection, m_view);
    }

    inline void updateKeyboardInput(Window& _window, float _deltaTime) {
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
        float rotation_speed = _deltaTime * _window.m_rotation_speed;
        if (glfwGetMouseButton(_window.getWindow(), GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
            m_cam_orientation.x -= rotation_speed * _window.getCursorVel().y;
            m_cam_orientation.y -= rotation_speed * _window.getCursorVel().x;
            Transformation::clampOrientation(m_cam_orientation);
            Transformation::getViewVectors(m_cam_orientation, m_front, m_right, m_real_up);
        }
    }

public:
    float m_free_cam_speed = 16.f;
    float m_fovy{M_PI_2f};
    glm::vec2 m_near_far{1.e-1f, 1.e8f};

    CamerableSystem() {
        Transformation::getViewVectors(m_cam_orientation, m_front, m_right, m_real_up);
        updateRenderingData(16.f / 9.f);
    }

    inline const glm::vec3& getCamPos() const { return m_cam_pos; }
    inline const glm::mat4& getView() const { return m_view; }
    inline const glm::mat4& getProjection() const { return m_projection; }
    inline const Frustum& getFrustum() const { return m_frustum; }
    inline ECS::ControlType getControlType() { return m_control_type; }
    inline std::optional<ECS::EntityId> getControlledEntity() { return m_controlled_entity; }

    inline void init(ComponentManager& cm, ECS::EntityId entity) {}
    inline void clear(ComponentManager& cm, ECS::EntityId entity) {
        if (m_controlled_entity.has_value() && m_controlled_entity.value() == entity) {
            stopControl();
        }
    }

    inline void startControl(ComponentManager& cm, ECS::EntityId _entity, Window& _window) {
        m_controlled_entity = _entity;
        ECS::Positionnable& positionnable = cm.getComponent<ECS::Positionnable>(_entity);
        ECS::Orientable& orientable = cm.getComponent<ECS::Orientable>(_entity);
        ECS::Camerable& camerable = cm.getComponent<ECS::Camerable>(_entity);
        m_cam_orientation = orientable.orientation;
        Transformation::getViewVectors(m_cam_orientation, m_front, m_right, m_real_up);
        applyPosConstraint(positionnable, camerable);
        updateRenderingData(_window.getAspectRatio());
    }
    inline void stopControl() {
        m_controlled_entity.reset();
    }

    inline void changeControlType(ComponentManager& cm, ECS::ControlType _new_type) {
        m_control_type = _new_type;

        if (!m_controlled_entity.has_value())
            return;
        ECS::EntityId entity = m_controlled_entity.value();
        ECS::Positionnable& positionnable = cm.getComponent<ECS::Positionnable>(entity);
        ECS::Camerable& camerable = cm.getComponent<ECS::Camerable>(entity);

        switch (m_control_type) {
        case ControlType::FreeCam:
            break;
        case ControlType::FirstPerson:
            m_cam_pos = positionnable.pos + camerable.eye_pos;
            break;
        case ControlType::ThirdPerson:
            // update target posm_zoom_rate
            m_cam_pos = positionnable.pos + camerable.eye_pos - camerable.distance_to_center * m_front;
            break;
        case ControlType::__COUNT:
            break;
        }
    }
    inline void toggleControlType(ComponentManager& cm) {
        changeControlType(cm, ECS::ControlType((int(m_control_type) + 1) % ECS::NB_CONTROL_TYPES));
    }

    void update(ComponentManager& cm, Window& _window, float _deltaTime) {
        if (m_controlled_entity.has_value() && m_control_type != ControlType::FreeCam) {
            ECS::EntityId entity = m_controlled_entity.value();
            ECS::Positionnable& positionnable = cm.getComponent<ECS::Positionnable>(entity);
            ECS::Orientable& orientable = cm.getComponent<ECS::Orientable>(entity);
            ECS::Camerable& camerable = cm.getComponent<ECS::Camerable>(entity);
            m_cam_orientation = orientable.orientation;
            Transformation::getViewVectors(m_cam_orientation, m_front, m_right, m_real_up);
            applyPosConstraint(positionnable, camerable);
        } else {
            updateKeyboardInput(_window, _deltaTime);
            updateMouseInput(_window, _deltaTime);
        }
        updateRenderingData(_window.getAspectRatio());
    }
};

class ECS::ControllingSystem : public SystemBase<ECS::Movable, ECS::Groundable, ECS::Orientable, ECS::Controllable> {
public:
    inline void init(ComponentManager& cm, ECS::EntityId entity) {}
    inline void clear(ComponentManager& cm, ECS::EntityId entity) {}

    inline void updateMouseInput(ECS::Orientable& orientable, Window& _window, float _deltaTime) {
        float rotation_speed = _deltaTime * _window.m_rotation_speed;
        if (glfwGetMouseButton(_window.getWindow(), GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
            orientable.orientation.x -= rotation_speed * _window.getCursorVel().y;
            orientable.orientation.y -= rotation_speed * _window.getCursorVel().x;
            Transformation::clampOrientation(orientable.orientation);
        }
    }

    inline void updateKeyboardInput(ECS::Movable& movable, ECS::Groundable& groundable, ECS::Orientable& orientable, Window& _window, float _deltaTime) {
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

    void update(ComponentManager& cm, Window& _window, float _deltaTime) {
        for (ECS::EntityId entity : m_entities) {
            // ECS::Positionnable& positionnable = cm.getComponent<ECS::Positionnable>(entity);
            ECS::Movable& movable = cm.getComponent<ECS::Movable>(entity);
            ECS::Groundable& groundable = cm.getComponent<ECS::Groundable>(entity);
            ECS::Orientable& orientable = cm.getComponent<ECS::Orientable>(entity);

            updateKeyboardInput(movable, groundable, orientable, _window, _deltaTime);
            updateMouseInput(orientable, _window, _deltaTime);
        }
    }
};

// -------------------------------------------------------------------------
// SYSTEM MANAGER
// -------------------------------------------------------------------------

class ECS::SystemManager {
    ECS::SystemList m_systems;

public:
    // Convenience function to get the statically casted pointer to the System of type T.
    template <ECS::System S>
    constexpr S& getSystem() { return std::get<ECS::system_id<S>>(m_systems); }
    template <ECS::System S>
    constexpr const S& getSystem() const { return std::get<ECS::system_id<S>>(m_systems); }

    // Called whenever an entity's signature changes (add/remove component, destroy)
    void onEntitySignatureChanged(ECS::ComponentManager& cm, ECS::EntityId _entity, const ECS::ComponentSignature& _entity_sig) {
        ECS::for_each_systems([&]<ECS::System S>() {
            S& system = getSystem<S>();
            if ((_entity_sig & system.signature) == system.signature) {
                system.m_entities.insert(_entity);
                system.init(cm, _entity);
            } else {
                if (system.m_entities.erase(_entity)) {
                    system.clear(cm, _entity);
                }
            }
        });
    }
};