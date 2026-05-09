#pragma once
#include "Component.hpp"
#include "Entity.hpp"
#include <unordered_set>

// -------------------------------------------------------------------------
// SYSTEMS
// -------------------------------------------------------------------------

template <ECS::Component... Cs>
struct ECS::SystemBase {
    // Built entirely at compile time from the component pack
    static constexpr ECS::ComponentSignature signature = []() {
        ECS::ComponentSignature sig{};
        (sig.set(ECS::component_id<Cs>), ...);
        return sig;
    }();

    std::unordered_set<ECS::EntityId> m_entities{};
};

class ECS::PhysicsSystem : public ECS::SystemBase<ECS::Positionnable, ECS::Movable, ECS::Groundable, ECS::PhysicsStats> {
public:
    void init(ComponentManager& cm, ECS::EntityId entity) {}
    void clear(ComponentManager& cm, ECS::EntityId entity) {}

    void update(ComponentManager& cm, float _deltaTime) {
        for (ECS::EntityId entity : m_entities) {
            ECS::Positionnable& position = cm.getComponent<ECS::Positionnable>(entity);
            ECS::Movable& velocity = cm.getComponent<ECS::Movable>(entity);
            ECS::Groundable& ground = cm.getComponent<ECS::Groundable>(entity);
            ECS::PhysicsStats& stats = cm.getComponent<ECS::PhysicsStats>(entity);

            std::vector<glm::vec3> forces;
            forces.reserve(3);
            if (ground.on_ground) {
                Block* in_block = position.current_chunk->getBlock(position.pos);
                Block* under_block = in_block->m_neighbours[2]; // 2 -> -y
                if (under_block == nullptr || !under_block->hasHitbox()) {
                    ground.on_ground = false;
                } else {
                    float static_friction = under_block->getCollisionStats()[2];
                    velocity.vel.x *= 1.f - static_friction;
                    velocity.vel.z *= 1.f - static_friction;
                }
            } else {
                forces.push_back(glm::vec3(0.f, -9.81f, 0.f) * stats.weight); // g
                Block* block = position.current_chunk->getBlock(position.pos);
                float densite_fluide = block->getDensity();
                if (densite_fluide > 0.f) {
                    forces.push_back(densite_fluide * -forces[0] * stats.volume / (stats.weight / stats.volume)); // flottaison
                    forces.push_back(densite_fluide * velocity.vel * -stats.drag);                                // drag
                }
            }

            // Add forces
            glm::vec3 m_accel(0.f, 0.f, 0.f);
            for (const glm::vec3& force : forces) {
                m_accel += force;
            }
            m_accel /= stats.weight;
            velocity.vel += _deltaTime * m_accel;
        }
    }
};

class ECS::WorldCollisionSystem : public ECS::SystemBase<ECS::Positionnable, ECS::Collisionnable, ECS::Movable, ECS::Groundable> {
    void bounce(float _static_friction, float _friction, float _restitution, const glm::vec3& _normal, glm::vec3& _vel) {
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
        Block* block{nullptr};
    };
    // Return true if it detected a collision
    bool detectCollision(float _deltaTime, CollisionsInfos& res, ECS::Positionnable& _position, ECS::Collisionnable& _bounding_box, ECS::Movable& _velocity) {
        res.t = std::numeric_limits<float>::max();

        std::vector<Block*> to_explore;
        for (const AABB<float>& hitbox : _bounding_box.hitboxes) {
            glm::ivec3 min_block = Block::posToBlockPos(_position.pos + hitbox.min);
            glm::ivec3 max_block = Block::posToBlockPos(_position.pos + hitbox.max);
            for (uint y : {min_block.y - 1, min_block.y, max_block.y, max_block.y + 1})
                for (uint z : {min_block.z - 1, min_block.z, max_block.z, max_block.z + 1})
                    for (uint x : {min_block.x - 1, min_block.x, max_block.x, max_block.x + 1})
                        to_explore.push_back(_position.current_chunk->findBlock(glm::ivec3(x, y, z)));
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
            for (const AABB<float>& hitbox : _bounding_box.hitboxes) {
                glm::vec3 dist;
                if (block_aabb.intersectAABB(_position.pos + hitbox, dist)) {
                    clip = true;
                    continue;
                }

                float t;
                glm::vec3 normal(0.);
                if (block->hasHitbox() && block_aabb.intersectAABB(_position.pos + hitbox, _deltaTime * _velocity.vel, t, normal)) {
                    if (t < res.t) {
                        res.t = t;
                        res.normal = normal;
                        res.block = block;
                        res.pos = _position.pos + t * _deltaTime * _velocity.vel;
                        while (block_aabb.intersectAABB(res.pos + hitbox, dist)) {
                            res.pos += dist;
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
        ECS::Positionnable& position = cm.getComponent<ECS::Positionnable>(_entity);
        ECS::Collisionnable& bounding_box = cm.getComponent<ECS::Collisionnable>(_entity);
        ECS::Movable& velocity = cm.getComponent<ECS::Movable>(_entity);
        ECS::Groundable& Ground = cm.getComponent<ECS::Groundable>(_entity);

        // Chunk collision detection
        CollisionsInfos collision;
        // std::cout << std::endl
        //           << std::endl
        //           << std::endl
        //           << "Starting detection: dt=" << _deltaTime << std::endl;
        while (detectCollision(_deltaTime, collision, position, bounding_box, velocity)) {
            // std::cout << "\tCOLLISION: " << std::endl
            //           << "\t\t" << size_t(collision.block->getType()) << " at " << glm::to_string(collision.block->getPos()) << std::endl
            //           << "\t\tt: " << collision.t << std::endl
            //           << "\t\tnormal: " << glm::to_string(collision.normal) << std::endl
            //           << "\t\told pos: " << glm::to_string(position.pos) << std::endl
            //           << "\t\told vel: " << glm::to_string(velocity.vel) << std::endl;

            float friction = collision.block->getCollisionStats()[0];
            float restitution = collision.block->getCollisionStats()[1];
            bounce(0.f, friction, restitution, collision.normal, velocity.vel);
            if (collision.normal == glm::vec3(0.f, 1.f, 0.f) && velocity.vel.y == 0.f) {
                Ground.on_ground = true;
            }
            position.pos = collision.pos;
            position.current_chunk = position.current_chunk->getChunk(position.pos);
            if (position.current_chunk == nullptr) {
                return;
            }
            _deltaTime *= (1.f - collision.t);
            // std::cout
            //     << "\t\tpos: " << glm::to_string(position.pos) << std::endl
            //     << "\t\tvel: " << glm::to_string(velocity.vel) << std::endl
            //     << "dt=" << _deltaTime << std::endl;
        }
        position.pos += _deltaTime * velocity.vel;
        // std::cout << "\tAPPLIED dt=" << _deltaTime << " velocity" << std::endl
        //           << "\t\tnew pos: " << glm::to_string(position.pos) << std::endl
        //           << "\t\tnew vel: " << glm::to_string(velocity.vel) << std::endl;

        position.current_chunk = position.current_chunk->getChunk(position.pos);
        if (position.current_chunk == nullptr) {
            return;
        }

        return;
    }

public:
    void init(ComponentManager& cm, ECS::EntityId entity) {}
    void clear(ComponentManager& cm, ECS::EntityId entity) {}

    void update(ComponentManager& cm, float _deltaTime) {
        for (ECS::EntityId entity : m_entities)
            updateEntity(cm, _deltaTime, entity);
    }
};

class ECS::HitBoxDisplaySystem : public ECS::SystemBase<ECS::Positionnable, ECS::Collisionnable, ECS::CollisionDisplay> {
public:
    void init(ComponentManager& cm, ECS::EntityId entity) {
        ECS::Collisionnable& collision = cm.getComponent<ECS::Collisionnable>(entity);
        for (AABB<float>& hitbox : collision.hitboxes)
            hitbox.initShaderData();
    }

    void render(ComponentManager& cm, ShaderProgram& _shader) const {
        _shader.set("color", glm::vec3(1.f));
        for (ECS::EntityId entity : m_entities) {
            ECS::Positionnable& position = cm.getComponent<ECS::Positionnable>(entity);
            ECS::Collisionnable& collision = cm.getComponent<ECS::Collisionnable>(entity);
            _shader.set("position", position.pos);
            for (AABB<float>& hitbox : collision.hitboxes)
                hitbox.render();
        }
    }

    void clear(ComponentManager& cm, ECS::EntityId entity) {
        ECS::Collisionnable& collision = cm.getComponent<ECS::Collisionnable>(entity);
        for (AABB<float>& hitbox : collision.hitboxes)
            hitbox.clearShaderData();
    }
};

class ECS::ControllingSystem : public ECS::SystemBase<ECS::Positionnable, ECS::Movable, ECS::Groundable, ECS::Orientable, ECS::Camerable, ECS::Controllable> {
    std::optional<ECS::EntityId> m_currently_controlled{};

    ControlType m_type{ControlType::ThirdPerson};
    float m_free_cam_speed = 16.f;
    float m_fovy{M_PI_2f};
    glm::vec2 m_near_far{1.e-1f, 1.e8f};

    glm::vec3 m_cam_pos;
    glm::vec3 m_front;
    glm::vec3 m_right;
    glm::vec3 m_real_up;
    glm::mat4 m_view;
    glm::mat4 m_projection;
    // Camera::Frustum m_frustum;

    void updateAngles(ECS::Orientable& orientable) {
        orientable.orientation.x = glm::clamp(orientable.orientation.x, -M_PI_2_SAFE, M_PI_2_SAFE);
        orientable.orientation.y = Transformation::clipAnglePI(orientable.orientation.y);
        m_front = glm::normalize(Transformation::EulerToEuclidian(orientable.orientation));
        m_right = glm::normalize(glm::cross(m_front, VEC_UP));
        m_real_up = glm::normalize(glm::cross(m_right, m_front));
    }

    void updateRenderingData(float _aspect_ratio) {
        m_projection = glm::perspective(m_fovy, _aspect_ratio, m_near_far[0], m_near_far[1]);
        m_view = glm::lookAt(m_cam_pos, m_cam_pos + m_front, m_real_up);
        // m_frustum.updatePlanes(this);
    }

    void changeType(ECS::Positionnable& positionnable, ECS::Camerable& camerable, ControlType _new_type) {
        m_type = _new_type;
        switch (m_type) {
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

    void applyPosConstraint(ECS::Positionnable& positionnable, ECS::Orientable& orientable, ECS::Camerable& camerable) {
        switch (m_type) {
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
            orientable.orientation = Transformation::EuclidianToEuler(m_front);
            updateAngles(orientable);
            break;
        case ControlType::__COUNT:
            break;
        }
    }

    void updateKeyboardInput(ECS::Positionnable& positionnable, ECS::Movable& movable, ECS::Groundable& groundable, ECS::Camerable& camerable, Window& _window, float _deltaTime) {
        glm::vec3 motion = glm::vec3(
            _window.keyboard.isHeld(GLFW_KEY_SPACE) - _window.keyboard.isHeld(GLFW_KEY_LEFT_CONTROL),
            _window.keyboard.isHeld(GLFW_KEY_D) - _window.keyboard.isHeld(GLFW_KEY_A),
            _window.keyboard.isHeld(GLFW_KEY_W) - _window.keyboard.isHeld(GLFW_KEY_S));
        glm::vec3 flat_front = glm::cross(VEC_UP, m_right);

        switch (m_type) {
        case ControlType::FreeCam:
            m_cam_pos += _deltaTime * m_free_cam_speed * (motion.x * VEC_UP + motion.y * m_right + motion.z * flat_front);
            break;
        case ControlType::FirstPerson:
        case ControlType::ThirdPerson:
            movable.vel += (groundable.on_ground ? groundable.walk_speed : groundable.air_control_speed) * (motion.y * m_right + motion.z * flat_front);
            if (_window.keyboard.getState(GLFW_KEY_SPACE).pressed && groundable.on_ground) {
                movable.vel += VEC_UP * groundable.jump_force;
                groundable.on_ground = false;
            }

            break;
        case ControlType::__COUNT:
            break;
        }
    }

    void updateMouseInput(ECS::Positionnable& positionnable, ECS::Orientable& orientable, ECS::Camerable& camerable, Window& _window, float _deltaTime) {
        float rotation_speed = _deltaTime * _window.m_rotation_speed;
        switch (m_type) {
        case ControlType::FreeCam:
        case ControlType::FirstPerson:
            if (glfwGetMouseButton(_window.getWindow(), GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
                orientable.orientation.x -= rotation_speed * _window.getCursorVel().y;
                orientable.orientation.y -= rotation_speed * _window.getCursorVel().x;
                updateAngles(orientable);
            }
            break;
        case ControlType::ThirdPerson:
            camerable.distance_to_center = glm::max(camerable.distance_to_center * (1.f - _window.getScroll().y * _window.m_zoom_rate), 1.e-4f);
            if (glfwGetMouseButton(_window.getWindow(), GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
                orientable.orientation.x -= rotation_speed * _window.getCursorVel().y;
                orientable.orientation.y -= rotation_speed * _window.getCursorVel().x;
                updateAngles(orientable);
                m_cam_pos = positionnable.pos + camerable.eye_pos - camerable.distance_to_center * m_front;
            }
            break;
        case ControlType::__COUNT:
            break;
        }
    }

public:
    inline const glm::vec3& getCamPos() const { return m_cam_pos; }
    inline const glm::mat4& getView() const { return m_view; }
    inline const glm::mat4& getProjection() const { return m_projection; }

    void init(ComponentManager& cm, ECS::EntityId entity) {}
    void clear(ComponentManager& cm, ECS::EntityId entity) {
        if (m_currently_controlled == entity) {
            stopControl();
        }
    }
    inline void startControl(ComponentManager& cm, Window& _window, ECS::EntityId entity) {
        assert(m_entities.find(entity) != m_entities.end());
        m_currently_controlled.emplace(entity);

        ECS::Positionnable& positionnable = cm.getComponent<ECS::Positionnable>(entity);
        // ECS::Movable& movable = cm.getComponent<ECS::Movable>(entity);
        // ECS::Groundable& groundable = cm.getComponent<ECS::Groundable>(entity);
        ECS::Orientable& orientable = cm.getComponent<ECS::Orientable>(entity);
        ECS::Camerable& camerable = cm.getComponent<ECS::Camerable>(entity);

        updateAngles(orientable);
        applyPosConstraint(positionnable, orientable, camerable);
        updateRenderingData(_window.getAspectRatio());

        _window.keyboard.bind(GLFW_KEY_C, [&]() { changeType(positionnable, camerable, ControlType((int(m_type) + 1) % ECS::NB_CONTROL_TYPES)); }, nullptr);
        _window.keyboard.bind(GLFW_KEY_W, nullptr, nullptr);
        _window.keyboard.bind(GLFW_KEY_A, nullptr, nullptr);
        _window.keyboard.bind(GLFW_KEY_S, nullptr, nullptr);
        _window.keyboard.bind(GLFW_KEY_D, nullptr, nullptr);
        _window.keyboard.bind(GLFW_KEY_SPACE, nullptr, nullptr);
        _window.keyboard.bind(GLFW_KEY_LEFT_CONTROL, nullptr, nullptr);
    }
    inline void stopControl() {
        m_currently_controlled.reset();
    }

    void update(ComponentManager& cm, Window& _window, float _deltaTime) {
        if (!m_currently_controlled.has_value())
            return;
        ECS::EntityId entity = m_currently_controlled.value();
        ECS::Positionnable& positionnable = cm.getComponent<ECS::Positionnable>(entity);
        ECS::Movable& movable = cm.getComponent<ECS::Movable>(entity);
        ECS::Groundable& groundable = cm.getComponent<ECS::Groundable>(entity);
        ECS::Orientable& orientable = cm.getComponent<ECS::Orientable>(entity);
        ECS::Camerable& camerable = cm.getComponent<ECS::Camerable>(entity);

        updateKeyboardInput(positionnable, movable, groundable, camerable, _window, _deltaTime);
        updateMouseInput(positionnable, orientable, camerable, _window, _deltaTime);
        applyPosConstraint(positionnable, orientable, camerable);
        updateRenderingData(_window.getAspectRatio());
        std::cout << "POS:\t" << glm::to_string(positionnable.pos) << std::endl
                  << "VEL:\t" << glm::to_string(movable.vel) << std::endl;
    }
};

// -------------------------------------------------------------------------
// MANAGER
// -------------------------------------------------------------------------

class SystemManager {
    ECS::SystemList m_systems;

public:
    // Convenience function to get the statically casted pointer to the System of type T.
    template <ECS::System S>
    constexpr S& getSystem() { return std::get<ECS::system_id<S>>(m_systems); }

    // Called whenever an entity's signature changes (add/remove component, destroy)
    void onEntitySignatureChanged(ComponentManager& cm, ECS::EntityId _entity, const ECS::ComponentSignature& _entity_sig) {
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