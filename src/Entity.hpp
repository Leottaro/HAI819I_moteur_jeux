#pragma once
#include "Chunk.hpp"
#include "AABB.hpp"
#include "RigidBody.hpp"
#include <set>
#include <algorithm>

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

    // Return true if it detected a bounce
    struct CollisionsInfos {
        glm::vec3 normal;
        float t;
        std::array<float, 2> block_stats;
    };
    bool detectCollision(float _deltaTime, float max_t, CollisionsInfos &res) {
        res.t = std::numeric_limits<float>::max();
        for (const AABB<float> &hitbox : m_hitbox) {
            glm::vec3 hitbox_offset;
            std::set<std::array<int, 6>> seen_path;
            for (hitbox_offset.y = hitbox.min.y; hitbox_offset.y <= hitbox.max.y + 1.f; hitbox_offset.y++) {
                for (hitbox_offset.z = hitbox.min.z; hitbox_offset.z <= hitbox.max.z + 1.f; hitbox_offset.z++) {
                    for (hitbox_offset.x = hitbox.min.x; hitbox_offset.x <= hitbox.max.x + 1.f; hitbox_offset.x++) {
                        glm::ivec3 start = Block::posToBlockPos(hitbox_offset + m_pos);
                        glm::ivec3 end = Block::posToBlockPos(hitbox_offset + m_pos + _deltaTime * m_vel);
                        if (!seen_path.insert({start.x, start.y, start.z, end.x, end.y, end.z}).second)
                            continue;

                        std::vector<Block *> solid_blocks;
                        m_current_chunk->findSolidBlocks(start, end, solid_blocks);
                        if (solid_blocks.empty())
                            continue;
                        glm::ivec3 block_pos = solid_blocks[0]->getPos();
                        AABB<float> block_aabb(block_pos, block_pos + glm::ivec3(1));

                        std::cout << "\t" << glm::to_string(start) << " -> " << glm::to_string(end) << " found " << solid_blocks.size() << " blocks: " << std::endl;
                        for (const Block *block : solid_blocks) {
                            std::cout << "\t\t" << size_t(block->getType()) << " at " << glm::to_string(block_pos) << std::endl;

                            float t;
                            glm::vec3 normal;
                            if (block_aabb.intersectAABB(m_pos + hitbox, _deltaTime * m_vel, t, normal) && t < res.t) {
                                std::cout << "\t\t\tintersection: " << "t=" << t << "\tnormal=" << glm::to_string(normal) << std::endl;
                                res.normal = normal;
                                res.t = t;
                                res.block_stats = block->getCollisionStats();
                            }
                        }
                    }
                }
            }
        }

        return res.t <= max_t;
    }

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
            m_hitbox = {AABB<float>(glm::vec3(-0.1f), glm::vec3(0.1f))};
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
            Block *in_block = m_current_chunk->getBlock(m_pos);
            Block *under_block = in_block->m_neighbours[2]; // 2 -> -y
            if (under_block == nullptr || !under_block->hasHitbox()) {
                m_on_ground = false;
            } else {
                float ground_friction = under_block->getCollisionStats()[0];
                m_vel *= ground_friction - 1.f;
            }
        }

        if (!m_on_ground) {
            forces.push_back(glm::vec3(0.f, -9.81f, 0.f) * m_weight); // g
            Block *block = m_current_chunk->getBlock(m_pos);
            float densite_fluide = block->getDensity();
            if (densite_fluide > 0.f) {
                forces.push_back(densite_fluide * -forces[0] * m_volume / (m_weight / m_volume)); // flottaison
                forces.push_back(densite_fluide * m_vel * -m_drag);                               // drag
            }
        }

        RigidBody::addForces(_deltaTime, forces);

        // Chunk collision detection
        CollisionsInfos collision;
        float remaining_t = 1.f;
        std::cout << std::endl
                  << std::endl
                  << std::endl
                  << "Starting detection: remaining_t=" << remaining_t << std::endl;
        while (detectCollision(_deltaTime, remaining_t, collision)) {
            std::cout << "\tCOLLISION: " << std::endl
                      << "\t\tt: " << collision.t << std::endl
                      << "\t\tnormal: " << glm::to_string(collision.normal) << std::endl;

            m_friction = collision.block_stats[0];
            m_restitution = collision.block_stats[1];
            RigidBody::bounce(0.f, collision.normal);
            m_pos += collision.t * _deltaTime * m_vel;
            remaining_t -= collision.t;
            std::cout << "remaining_t=" << remaining_t << std::endl;
        }
        m_pos += remaining_t * _deltaTime * m_vel;

        if (m_camera != nullptr) {
            m_camera->updatePosConstraint();
            m_camera->updateData();
        }

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
