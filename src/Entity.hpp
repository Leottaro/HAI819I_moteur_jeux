#pragma once
#include "Chunk.hpp"
#include "AABB.hpp"
#include "RigidBody.hpp"
#include <set>
#include <algorithm>

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

    // Return true if it detected a collision
    struct CollisionsInfos {
        float t;
        glm::vec3 normal;
        glm::vec3 pos;
        Block *block;
    };
    bool detectCollision(float _deltaTime, CollisionsInfos &res) {
        res.t = std::numeric_limits<float>::max();

        std::vector<Block *> to_explore;
        for (const AABB<float> &hitbox : m_hitbox) {
            glm::ivec3 min_block = Block::posToBlockPos(m_pos + hitbox.min);
            glm::ivec3 max_block = Block::posToBlockPos(m_pos + hitbox.max);
            for (uint y : {min_block.y - 1, min_block.y, max_block.y, max_block.y + 1})
                for (uint z : {min_block.z - 1, min_block.z, max_block.z, max_block.z + 1})
                    for (uint x : {min_block.x - 1, min_block.x, max_block.x, max_block.x + 1})
                        to_explore.push_back(m_current_chunk->findBlock(glm::ivec3(x, y, z)));
        }

        std::set<Block *> explored;
        while (!to_explore.empty()) {
            Block *block = to_explore.back();
            to_explore.pop_back();
            if (!explored.insert(block).second)
                continue;

            glm::ivec3 block_pos = block->getPos();
            AABB<float> block_aabb(block_pos, block_pos + glm::ivec3(1));
            // std::cout << "\t\t" << size_t(block->getType()) << " at " << glm::to_string(block_pos) << std::endl;

            bool clip = false;
            for (const AABB<float> &hitbox : m_hitbox) {
                glm::vec3 dist;
                if (block_aabb.intersectAABB(m_pos + hitbox, dist)) {
                    clip = true;
                    continue;
                }

                float t;
                glm::vec3 normal(0.);
                if (block->hasHitbox() && block_aabb.intersectAABB(m_pos + hitbox, _deltaTime * m_vel, t, normal)) {
                    if (t < res.t) {
                        res.t = t;
                        res.normal = normal;
                        res.block = block;
                        res.pos = m_pos + t * _deltaTime * m_vel;
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
        } else {
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
        // std::cout << std::endl
        //           << std::endl
        //           << std::endl
        //           << "Starting detection: dt=" << _deltaTime << std::endl;
        while (detectCollision(_deltaTime, collision)) {
            // std::cout << "\tCOLLISION: " << std::endl
            //           << "\t\t" << size_t(collision.block->getType()) << " at " << glm::to_string(collision.block->getPos()) << std::endl
            //           << "\t\tt: " << collision.t << std::endl
            //           << "\t\tnormal: " << glm::to_string(collision.normal) << std::endl
            //           << "\t\told pos: " << glm::to_string(m_pos) << std::endl
            //           << "\t\told vel: " << glm::to_string(m_vel) << std::endl;

            m_friction = collision.block->getCollisionStats()[0];
            m_restitution = collision.block->getCollisionStats()[1];
            RigidBody::bounce(0.f, collision.normal);
            if (collision.normal == glm::vec3(0.f, 1.f, 0.f) && m_vel.y == 0.f) {
                m_on_ground = true;
            }
            m_pos = collision.pos;
            m_current_chunk = m_current_chunk->getChunk(m_pos);
            if (m_current_chunk == nullptr) {
                return false;
            }
            _deltaTime *= (1.f - collision.t);
            // std::cout
            //     << "\t\tpos: " << glm::to_string(m_pos) << std::endl
            //     << "\t\tvel: " << glm::to_string(m_vel) << std::endl
            //     << "dt=" << _deltaTime << std::endl;
        }
        m_pos += _deltaTime * m_vel;
        // std::cout << "\tAPPLIED dt=" << _deltaTime << " velocity" << std::endl
        //           << "\t\tnew pos: " << glm::to_string(m_pos) << std::endl
        //           << "\t\tnew vel: " << glm::to_string(m_vel) << std::endl;

        m_current_chunk = m_current_chunk->getChunk(m_pos);
        if (m_current_chunk == nullptr) {
            return false;
        }

        if (m_camera != nullptr) {
            m_camera->updatePosConstraint();
            m_camera->updateData();
        }

        return true;
    }

    void initShaderData() {
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
