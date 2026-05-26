#pragma once
#include <imgui.h>

#include "Data.hpp"

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

    std::vector<ECS::EntityId> getOutOfBoundEntities(ComponentManager& cm, const World* _world) const {
        std::vector<ECS::EntityId> entities;
        for (ECS::EntityId entity : m_entities) {
            if (cm.hasComponent<ECS::Controllable>(entity))
                continue;
            const ECS::Positionnable& positionnable = cm.getComponent<ECS::Positionnable>(entity);
            if (_world->findChunk(Chunk::posToChunkPos(positionnable.pos)) == nullptr)
                entities.push_back(entity);
        }

        return entities;
    }
};

class ECS::PhysicsSystem : public SystemBase<ECS::Positionnable, ECS::Movable, ECS::Collisionnable, ECS::Groundable, ECS::PhysicsStats> {
public:
    inline void init(ComponentManager& cm, ECS::EntityId entity) {}
    inline void clear(ComponentManager& cm, ECS::EntityId entity) {}

    void update(ComponentManager& cm, const World* _world, float _deltaTime) {
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
                            const Block* block = _world->findBlock(under_block_pos);
                            groundable.on_ground = groundable.on_ground || (block != nullptr && block->hasHitbox());
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
                const Block* block = _world->findBlock(positionnable.pos);
                if (block != nullptr) {
                    float densite_fluide = block->getDensity();
                    if (densite_fluide > 0.f) {
                        float entity_density = (groundable.wants_to_float ? .75f : 1.f) * (stats.weight / stats.volume);
                        acceleration += densite_fluide * -gravity * stats.volume / entity_density; // flottaison
                        acceleration += densite_fluide * movable.vel * -stats.drag;                // drag
                    }
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
    bool detectCollision(const World* _world, float _deltaTime, CollisionsInfos& res, const ECS::Positionnable& _positionnable, const ECS::Collisionnable& _collisionnable, const ECS::Movable& _movable) {
        res.t = std::numeric_limits<float>::max();

        std::vector<const Block*> to_explore;
        for (const AABB<float>& hitbox : _collisionnable.hitboxes) {
            glm::ivec3 min_block = Block::posToBlockPos(_positionnable.pos + hitbox.min);
            glm::ivec3 max_block = Block::posToBlockPos(_positionnable.pos + hitbox.max);
            for (uint y : {min_block.y - 1, min_block.y, max_block.y, max_block.y + 1})
                for (uint z : {min_block.z - 1, min_block.z, max_block.z, max_block.z + 1})
                    for (uint x : {min_block.x - 1, min_block.x, max_block.x, max_block.x + 1}) {
                        const Block* block = _world->findBlock(glm::ivec3(x, y, z));
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
                if (!block->hasHitbox() || !block_aabb.intersectAABB(_positionnable.pos + hitbox, _deltaTime * _movable.vel, t, normal) || t >= res.t)
                    continue;

                res.t = t;
                res.normal = normal;
                res.pos = _positionnable.pos + t * _deltaTime * _movable.vel;

                AABB<float> test(block_aabb);
                test.min -= 1.e-2f;
                test.max += 1.e-2f;
                while (test.intersectAABB(res.pos + hitbox, dist)) {
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
                            const Block* block = _world->findBlock(collision_block);
                            if (block == nullptr || !block->hasHitbox())
                                continue;
                            res.friction = std::max(res.friction, block->getFriction());
                            res.restitution = std::min(res.restitution, block->getRestitution());
                        }
                    }
                }
                // std::cout << "\t\t\tintersection: " << "t=" << res.t << "\tnormal=" << glm::to_string(res.normal) << "\tpos=" << glm::to_string(res.pos) << std::endl;
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

    void updateEntity(ComponentManager& cm, const World* _world, float _deltaTime, ECS::EntityId _entity) {
        ECS::Positionnable& positionnable = cm.getComponent<ECS::Positionnable>(_entity);
        const ECS::Collisionnable& collisionable = cm.getComponent<ECS::Collisionnable>(_entity);
        ECS::Movable& movable = cm.getComponent<ECS::Movable>(_entity);
        ECS::Groundable& groundable = cm.getComponent<ECS::Groundable>(_entity);
        bool is_controllable = cm.hasComponent<ECS::Controllable>(_entity);

        // Chunk collision detection
        CollisionsInfos collision;
        // std::cout << std::endl
        //           << std::endl
        //           << std::endl
        //           << "Starting detection: dt=" << _deltaTime << std::endl;
        while ((std::abs(movable.vel.x) > 1.e-4f || std::abs(movable.vel.y) > 1.e-4f || std::abs(movable.vel.z) > 1.e-4f) && detectCollision(_world, _deltaTime, collision, positionnable, collisionable, movable)) {
            // std::cout << "\tCOLLISION: " << std::endl
            //           << "\t\tat " << glm::to_string(collision.pos) << std::endl
            //           << "\t\tt: " << collision.t << std::endl
            //           << "\t\tnormal: " << glm::to_string(collision.normal) << std::endl
            //           << "\t\told pos: " << glm::to_string(positionnable.pos) << std::endl
            //           << "\t\told vel: " << glm::to_string(movable.vel) << std::endl;

            if (!is_controllable || collision.normal == glm::vec3(0.f, 1.f, 0.f))
                bounce(0.1f, collision.friction, collision.restitution, collision.normal, movable.vel);
            else
                bounce(0.f, 0.f, 0.f, collision.normal, movable.vel);

            if (collision.normal == glm::vec3(0.f, 1.f, 0.f) && movable.vel.y <= -0.05f * ECS::G.y) {
                groundable.on_ground = true;
                collision.pos.y += 1.e-2f;
            }

            if (_world->findChunk(Chunk::posToChunkPos(collision.pos)) == nullptr)
                return;
            positionnable.pos = collision.pos;
            _deltaTime *= (1.f - collision.t);
            // std::cout
            //     << "\t\tpos: " << glm::to_string(positionnable.pos) << std::endl
            //     << "\t\tvel: " << glm::to_string(movable.vel) << std::endl
            //     << "dt=" << _deltaTime << std::endl;
        }

        glm::vec3 final_pos = positionnable.pos + _deltaTime * movable.vel;
        if (_world->findBlock(Block::posToBlockPos(final_pos)) != nullptr)
            positionnable.pos = final_pos;
        // std::cout << "\tAPPLIED dt=" << _deltaTime << " velocity" << std::endl
        //           << "\t\tnew pos: " << glm::to_string(positionnable.pos) << std::endl
        //           << "\t\tnew vel: " << glm::to_string(movable.vel) << std::endl;
    }

public:
    inline void init(ComponentManager& cm, ECS::EntityId entity) {}
    inline void clear(ComponentManager& cm, ECS::EntityId entity) {}

    inline void update(ComponentManager& cm, const World* _world, float _deltaTime) {
        for (ECS::EntityId entity : m_entities)
            updateEntity(cm, _world, _deltaTime, entity);
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

class ECS::ControllingSystem : public SystemBase<ECS::Positionnable, ECS::Orientable, ECS::Controllable, ECS::Movable, ECS::Groundable, ECS::WorldInteraction> {
    std::optional<ECS::EntityId> m_controlled_entity{};
    std::optional<glm::ivec3> m_aimed_block{};

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

    inline void update(ComponentManager& cm, const Window& _window, const World* _world, float _deltaTime) {
        if (!m_controlled_entity.has_value())
            return;

        ECS::EntityId entity = m_controlled_entity.value();
        ECS::Controllable& controllable = cm.getComponent<ECS::Controllable>(entity);
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
        if (_window.keyboard.isHeld(GLFW_KEY_SPACE)) {
            groundable.wants_to_float = true;
            if (groundable.on_ground) {
                movable.vel += MathHelpers::VEC_UP * groundable.jump_force;
                groundable.on_ground = false;
            }
        } else {
            groundable.wants_to_float = false;
        }

        if (_window.getScroll().y < 0) {
            uint8_t new_block = static_cast<uint8_t>(controllable.block_in_hand);
            new_block++;
            if (new_block >= BLOCK_TYPES_N)
                new_block = 1;
            controllable.block_in_hand = static_cast<BlockType>(new_block);
            std::cout << "nouveau bloc : " << BLOCK_NAMES[new_block] << std::endl;
        } else if (_window.getScroll().y > 0) {
            uint8_t new_block = static_cast<uint8_t>(controllable.block_in_hand);
            new_block--;
            if (new_block == 0)
                new_block = BLOCK_TYPES_N - 1;
            controllable.block_in_hand = static_cast<BlockType>(new_block);
            std::cout << "nouveau bloc : " << BLOCK_NAMES[new_block] << std::endl;
        }

        const ECS::Positionnable& positionnable = cm.getComponent<ECS::Positionnable>(entity);
        std::vector<const Block*> block_line = _world->findBlockLine(positionnable.pos + orientable.eye_pos, positionnable.pos + orientable.eye_pos + 5.f * front);
        size_t i = 0;
        while (i < block_line.size() && (block_line[i] == nullptr || !block_line[i]->hasHitbox()))
            i++;

        if (i == block_line.size() || block_line[i] == nullptr) {
            m_aimed_block.reset();
            return;
        }

        Block block = block_line[i]->shallowCopy();
        m_aimed_block = block.getPos();

        ECS::WorldInteraction& world_interaction = cm.getComponent<ECS::WorldInteraction>(entity);
        uint64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
        if (!world_interaction.blocks_to_change.empty() ||
            now - world_interaction.last_interaction_time <= ECS::WORLD_INTERACTION_COOLDOWN)
            return;

        if (_window.mouse.isHeld(GLFW_MOUSE_BUTTON_LEFT) && _window.mouse.isHeld(GLFW_MOUSE_BUTTON_RIGHT)) {
            block.getType() = controllable.block_in_hand;
            world_interaction.blocks_to_change.push_back(block);
            world_interaction.last_interaction_time = now;
        } else if (_window.mouse.isHeld(GLFW_MOUSE_BUTTON_LEFT)) {
            block.getType() = BlockType::Air;
            world_interaction.blocks_to_change.push_back(block);
            world_interaction.last_interaction_time = now;
        } else if (_window.mouse.isHeld(GLFW_MOUSE_BUTTON_RIGHT) && i > 0) {
            Block before_block = block_line[i - 1]->shallowCopy();
            before_block.getType() = controllable.block_in_hand;
            world_interaction.blocks_to_change.push_back(before_block);
            world_interaction.last_interaction_time = now;
        }
    }

    inline void render(const ComponentManager& cm, ShaderProgram& _shader) const {
        if (!m_aimed_block.has_value())
            return;
        AABB<float> block_aabb(glm::vec3(0.f), glm::vec3(1.f));
        AABBRenderer renderer(block_aabb);

        _shader.set("color", glm::vec3(0.5f));
        _shader.set("position", glm::vec3(m_aimed_block.value()));
        renderer.render();
    }
};

class ECS::WorldInteractorSystem : public SystemBase<ECS::WorldInteraction> {
public:
    inline void init(ComponentManager& cm, ECS::EntityId entity) {}
    inline void clear(ComponentManager& cm, ECS::EntityId entity) {
        ECS::WorldInteraction& world_interaction = cm.getComponent<ECS::WorldInteraction>(entity);
        world_interaction.blocks_to_change.clear();
    }

    inline void updateWorld(const ComponentManager& cm, World* world) const {
        for (ECS::EntityId entity : m_entities) {
            const ECS::WorldInteraction& world_interaction = cm.getComponent<ECS::WorldInteraction>(entity);
            for (const Block& block : world_interaction.blocks_to_change) {
                glm::ivec3 chunk_pos = Chunk::blockPosToChunkPos(block.getPos());
                Chunk* chunk = world->findChunk(chunk_pos);
                if (chunk != nullptr)
                    chunk->setBlockType(block.getPos(), block.getType());
            }
        }
    }

    inline void clearUpdates(ComponentManager& cm) {
        for (ECS::EntityId entity : m_entities)
            cm.getComponent<ECS::WorldInteraction>(entity).blocks_to_change.clear();
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