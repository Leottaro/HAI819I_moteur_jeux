#pragma once
#include "Chunk.hpp"
#include "AABB.hpp"
#include "RigidBody.hpp"
#include <set>

struct FloatRange {
    float current, b;
    static constexpr float tol = 1e-6f;

    FloatRange(float a, float b) : current(a), b(b) {}

    struct Iterator {
        float current, b;
        float operator*() const { return std::min(current, b); }
        Iterator &operator++() {
            current += 1.0f;
            return *this;
        }
        bool operator!=(const Iterator &) const { return current <= b + tol; }
    };

    Iterator begin() { return {current, b}; }
    Iterator end() { return {b, b}; }
};

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
            m_hitbox = {AABB<float>(glm::vec3(-0.2f), glm::vec3(0.2f))};
            // m_hitbox = {AABB<float>(glm::vec3(-1.f / 3.f, 0.f, -1.f / 3.f), glm::vec3(1.f / 3.f, 1.74f, 1.f / 3.f))};
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

        if (m_on_ground) {
            Block &in_block = m_current_chunk->getBlock(m_pos);
            Block *under_block = in_block.m_neighbours[2]; // 2 -> -y
            if (under_block == nullptr || !under_block->hasHitbox()) {
                m_on_ground = false;
            } else {
                float ground_friction = under_block->getCollisionStats()[0];
                m_vel *= ground_friction - 1.f;
            }
        }

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
        std::array<float, 2> collision_block_stats;
        for (const AABB<float> &hitbox : m_hitbox) {
            glm::vec3 hitbox_offset;
            std::set<std::array<int, 6>> seen_path;
            for (hitbox_offset.y = hitbox.min.y; hitbox_offset.y <= hitbox.max.y + 1.f; hitbox_offset.y++) {
                for (hitbox_offset.z = hitbox.min.z; hitbox_offset.z <= hitbox.max.z + 1.f; hitbox_offset.z++) {
                    for (hitbox_offset.x = hitbox.min.x; hitbox_offset.x <= hitbox.max.x + 1.f; hitbox_offset.x++) {
                        glm::ivec3 start = hitbox_offset + old_pos;
                        glm::ivec3 end = hitbox_offset + m_pos;
                        Block *inside_block = m_current_chunk->findFirstSolidBlock(start, end);
                        if (inside_block == nullptr || !seen_path.insert({start.x, start.y, start.z, end.x, end.y, end.z}).second) {
                            continue;
                        }
                        glm::ivec3 block_pos = inside_block->getPos();
                        AABB<float> block_aabb(block_pos, block_pos + glm::ivec3(1));

                        std::cout << glm::to_string(start) << " -> " << glm::to_string(end) << ": " << std::endl
                                  << "\tfound block " << size_t(inside_block->getType()) << " at " << glm::to_string(block_pos) << std::endl;
                        float t;
                        glm::vec3 normal;
                        if (block_aabb.intersectAABB(old_pos + hitbox, _deltaTime * m_vel, t, normal) && t < collision_t) {
                            collision_t = t;
                            collision_normal = normal;
                            collision_block_stats = inside_block->getCollisionStats();
                        }
                    }
                }
            }
        }

        if (collision_t < FLT_MAX) {
            std::cout << "COLLISION: " << std::endl
                      << "\tt: " << collision_t << std::endl
                      << "\tnormal: " << glm::to_string(collision_normal) << std::endl;
            // m_vel = collision_t * m_vel;
            // m_pos = old_pos + _deltaTime * m_vel;

            // if (collision_normal.x != 0.f && std::abs(m_vel.x) < 1.e-4f) {
            //     m_vel.x = 0.f;
            // }
            // if (collision_normal.y != 0.f && std::abs(m_vel.y) < 1.e-4f) {
            //     m_vel.y = 0.f;
            //     if (collision_normal == glm::vec3(0.f, 1.f, 0.f)) {
            //         m_on_ground = true;
            //     }
            // }
            // if (collision_normal.z != 0.f && std::abs(m_vel.z) < 1.e-4f) {
            //     m_vel.z = 0.f;
            // }

            m_friction = collision_block_stats[0];
            m_restitution = collision_block_stats[1];
            RigidBody::bounce(0.f, collision_normal);
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
