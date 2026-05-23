#pragma once
#include "Data.hpp"

#include <imgui.h>

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
            const ECS::Positionnable& positionnable = cm.getComponent<ECS::Positionnable>(entity);
            if (positionnable.current_world->findChunk(Chunk::posToChunkPos(positionnable.pos)) == nullptr)
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
            const ECS::Positionnable& positionnable = cm.getComponent<ECS::Positionnable>(entity);
            ECS::Movable& movable = cm.getComponent<ECS::Movable>(entity);
            const ECS::Collisionnable& collisionable = cm.getComponent<ECS::Collisionnable>(entity);
            ECS::Groundable& groundable = cm.getComponent<ECS::Groundable>(entity);
            const ECS::PhysicsStats& stats = cm.getComponent<ECS::PhysicsStats>(entity);

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
                            const Block* block = positionnable.current_world->findBlock(under_block_pos);
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
                float densite_fluide = positionnable.current_world->findBlock(positionnable.pos)->getDensity();
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
    bool detectCollision(float _deltaTime, CollisionsInfos& res, const ECS::Positionnable& _positionnable, const ECS::Collisionnable& _collisionnable, const ECS::Movable& _movable) {
        res.t = std::numeric_limits<float>::max();

        std::vector<const Block*> to_explore;
        for (const AABB<float>& hitbox : _collisionnable.hitboxes) {
            glm::ivec3 min_block = Block::posToBlockPos(_positionnable.pos + hitbox.min);
            glm::ivec3 max_block = Block::posToBlockPos(_positionnable.pos + hitbox.max);
            for (uint y : {min_block.y - 1, min_block.y, max_block.y, max_block.y + 1})
                for (uint z : {min_block.z - 1, min_block.z, max_block.z, max_block.z + 1})
                    for (uint x : {min_block.x - 1, min_block.x, max_block.x, max_block.x + 1}) {
                        const Block* block = _positionnable.current_world->findBlock(glm::ivec3(x, y, z));
                        if (block != nullptr)
                            to_explore.push_back(block);
                    }
        }

        std::set<const Block*> explored;
        while (!to_explore.empty()) {
            const Block* block = to_explore.back();
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
                                    const Block* block = _positionnable.current_world->findBlock(collision_block);
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
        const ECS::Collisionnable& collisionable = cm.getComponent<ECS::Collisionnable>(_entity);
        ECS::Movable& movable = cm.getComponent<ECS::Movable>(_entity);
        ECS::Groundable& groundable = cm.getComponent<ECS::Groundable>(_entity);
        positionnable.last_pos = positionnable.pos;

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
            if (positionnable.current_world->findChunk(Chunk::posToChunkPos(positionnable.pos)) == nullptr) {
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

        if (positionnable.current_world->findChunk(Chunk::posToChunkPos(positionnable.pos)) == nullptr) {
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
        const ECS::Collisionnable& collisionnable = cm.getComponent<ECS::Collisionnable>(entity);
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

class ECS::OrientationDisplaySystem : public SystemBase<ECS::Positionnable, ECS::Orientable, ECS::OrientationDisplay> {
    inline static GLuint LINE_VAO{0};
    inline static GLuint LINE_VBO{0};

public:
    inline void init(ComponentManager& cm, ECS::EntityId entity) {}
    inline void clear(ComponentManager& cm, ECS::EntityId entity) {}

    inline void render(const ComponentManager& cm, ShaderProgram& _shader) const {
        if (LINE_VAO == 0) {
            glGenVertexArrays(1, &LINE_VAO);
        }
        glBindVertexArray(LINE_VAO);

        if (LINE_VBO == 0) {
            glGenBuffers(1, &LINE_VBO);
            glBindBuffer(GL_ARRAY_BUFFER, LINE_VBO);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
        } else {
            glBindBuffer(GL_ARRAY_BUFFER, LINE_VBO);
        }

        std::vector<glm::vec3> points;
        points.reserve(m_entities.size() * 2);

        for (ECS::EntityId entity : m_entities) {
            const ECS::Positionnable& positionnable = cm.getComponent<ECS::Positionnable>(entity);
            const ECS::Orientable& orientable = cm.getComponent<ECS::Orientable>(entity);
            glm::vec3 front, right, real_up;
            Transformation::getViewVectors(orientable.orientation, front, right, real_up);

            points.push_back(positionnable.pos + orientable.eye_pos);
            points.push_back(positionnable.pos + orientable.eye_pos + glm::normalize(front));
        }

        glBufferData(GL_ARRAY_BUFFER, points.size() * sizeof(glm::vec3), points.data(), GL_DYNAMIC_DRAW);
        _shader.set("color", glm::vec3(0.f, 0.f, 1.f));
        _shader.set("position", glm::vec3(0.f));
        glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(points.size()));
    }
};

class ECS::ControllingSystem : public SystemBase<ECS::Positionnable, ECS::Orientable, ECS::Controllable, ECS::Movable, ECS::Groundable> {
    std::optional<ECS::EntityId> m_controlled_entity{};

public:
    inline void init(ComponentManager& cm, ECS::EntityId entity) {}
    inline void clear(ComponentManager& cm, ECS::EntityId entity) {
        if (m_controlled_entity.has_value() && m_controlled_entity.value() == entity) {
            stopControl();
        }
    }

    inline void startControl(ECS::EntityId _entity) { m_controlled_entity = _entity; }
    inline void stopControl() { m_controlled_entity.reset(); }

    inline void changeControlType(ComponentManager& cm, ECS::ControlType _new_type) {
        if (m_controlled_entity.has_value()) {
            cm.getComponent<ECS::Controllable>(m_controlled_entity.value()).type = _new_type;
        }
    }
    inline void toggleControlType(ComponentManager& cm) {
        if (m_controlled_entity.has_value()) {
            ECS::ControlType& control_type = cm.getComponent<ECS::Controllable>(m_controlled_entity.value()).type;
            control_type = static_cast<ECS::ControlType>((static_cast<int>(control_type) + 1) % ECS::NB_CONTROL_TYPES);
        }
    }

    inline std::optional<ECS::EntityId> getControlledEntity() const { return m_controlled_entity; }

    inline void update(ComponentManager& cm, Window& _window, float _deltaTime) {
        if (!m_controlled_entity.has_value())
            return;

        ECS::EntityId entity = m_controlled_entity.value();
        const ECS::Controllable& controllable = cm.getComponent<ECS::Controllable>(entity);
        if (controllable.type == ControlType::FreeCam)
            return;
        ECS::Movable& movable = cm.getComponent<ECS::Movable>(entity);
        ECS::Groundable& groundable = cm.getComponent<ECS::Groundable>(entity);
        const ECS::Orientable& orientable = cm.getComponent<ECS::Orientable>(entity);

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

    friend ECSManager;
};