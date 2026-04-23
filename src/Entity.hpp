#pragma once
#include "Chunk.hpp"
#include "AABB.hpp"
#include "RigidBody.hpp"
#include <unordered_set>

class Entity : public RigidBody {
public:
    static constexpr float JUMP_FORCE = 9.f;
    static constexpr float WALK_SPEED = 4.5f;

    enum class Type {
        Test
    };

private:
    GLuint m_VAO = 0;
    GLuint m_VBO = 0;
    GLuint m_EBO = 0;

    std::string m_uuid;
    std::vector<AABB<float>> m_hitbox;
    Chunk *m_current_chunk;
    bool m_on_ground;

public:
    Camera *m_camera;

    Entity(Entity &&) = delete;
    Entity(const Entity &) = delete;
    Entity &operator=(const Entity &) = delete;
    Entity &operator=(Entity &&) = delete;
    Entity(Type _type, const std::string &_uuid, Chunk *_current_chunk, const glm::vec3 &_pos) {
        m_pos = _pos;
        m_uuid = _uuid;
        m_current_chunk = _current_chunk;

        switch (_type) {
        case Type::Test:
            m_hitbox = {AABB<float>(glm::vec3(-0.5f), glm::vec3(0.5f))};
            // m_hitbox = {AABB<float>(glm::vec3(-0.3333333f, 0.f, -0.3333333f), glm::vec3(0.3333333f, 1.74f, 0.3333333f))};
            break;
        }

        initShaderData();
    }
    ~Entity() { clearShaderData(); }

    void fixCamera(Camera *_camera) {
        m_camera = _camera;
        m_camera->m_center = &m_pos;
        _camera->updatePosConstraint();
        _camera->updateData();
    }

    bool update(float _deltaTime) {
        if (m_current_chunk == nullptr)
            return false;

        std::vector<glm::vec3> forces;
        forces.reserve(3);

        if (!m_on_ground) {
            forces.push_back(glm::vec3(0.f, -9.81f, 0.f) * m_weight); // g
            Block &block = m_current_chunk->getBlock(m_pos);
            float densite_fluide = block.getDensity();
            if (densite_fluide > 0.f) {
                forces.push_back(densite_fluide * -forces[0] * m_volume / (m_weight / m_volume)); // flottaison
                forces.push_back(densite_fluide * m_vel * -m_drag);                               // drag
            }
        }

        glm::vec3 old_pos = m_pos;
        RigidBody::update(_deltaTime, forces);

        // Chunk collision detection
        float collision_t = FLT_MAX;
        glm::vec3 collision_normal;
        Block::Type collision_block_type;
        glm::ivec3 old_pos_block(old_pos), m_pos_block(m_pos);
        for (const AABB<float> &hitbox : m_hitbox) {
            glm::ivec3 hitbox_min(glm::floor(hitbox.min)), hitbox_max(glm::floor(hitbox.max));
            glm::ivec3 delta;
            std::unordered_set<Block *> checked_block;
            for (delta.y = hitbox_min.y; delta.y <= hitbox_max.y; delta.y++) {
                for (delta.z = hitbox_min.z; delta.z <= hitbox_max.z; delta.z++) {
                    for (delta.x = hitbox_min.x; delta.x <= hitbox_max.x; delta.x++) {
                        Block *inside_block = m_current_chunk->getFirstSolidBlock(old_pos_block + delta, m_pos_block + delta);
                        if (inside_block == nullptr || !checked_block.insert(inside_block).second) {
                            continue;
                        }
                        glm::vec3 block_pos = inside_block->getPos();
                        AABB<float> block_aabb(block_pos, block_pos + glm::vec3(1.f));

                        float t;
                        glm::vec3 normal;
                        if (block_aabb.intersectAABB(old_pos + hitbox, _deltaTime * m_vel, t, normal) && t < collision_t) {
                            collision_t = t;
                            collision_normal = normal;
                            collision_block_type = inside_block->getType();
                        }
                    }
                }
            }
        }

        if (collision_t < FLT_MAX) {
            m_vel = collision_t * m_vel;
            m_pos = old_pos + _deltaTime * m_vel;
            auto [friction, bounciness] = Block::PHYSICS_TABLE[size_t(collision_block_type)];
            if (bounciness > 0.f) {
                m_restitution = bounciness;
                RigidBody::bounce(friction, collision_normal);
            } else {
                if (collision_normal == glm::vec3(0.f, 1.f, 0.f)) {
                    m_on_ground = true;
                }
            }
        }

        if (m_camera != nullptr)
            m_camera->updatePosConstraint();

        m_current_chunk = m_current_chunk->getChunk(m_pos);
        if (m_current_chunk == nullptr) {
            return false;
        }

        return true;
    }

    void
    initShaderData() {
        for (size_t i = 0; i < m_hitbox.size(); i++) {
            m_hitbox[i].initShaderData();
        }
    }

    void updateShaderData() {
        for (size_t i = 0; i < m_hitbox.size(); i++) {
            m_hitbox[i].updateShaderData();
        }
    }

    void render() const {
        for (size_t i = 0; i < m_hitbox.size(); i++) {
            m_hitbox[i].render();
        }
    }

    void clearShaderData() {
        for (size_t i = 0; i < m_hitbox.size(); i++) {
            m_hitbox[i].clearShaderData();
        }
    }
};
