#pragma once
#include "Chunk.hpp"
#include "AABB.hpp"
#include "RigidBody.hpp"
// #include <cstddef>

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
            m_hitbox = {AABB<float>(glm::vec3(-0.49f, 0.f, -0.49f), glm::vec3(0.49f, 1.74f, 0.49f))};
            break;
        }

        initShaderData();
    }
    ~Entity() { clearShaderData(); }

    void update(float _deltaTime) {
        if (m_on_ground)
            return;

        std::vector<glm::vec3> forces;
        forces.reserve(3);

        forces.push_back(glm::vec3(0.f, -9.81f, 0.f) * m_weight);

        Block &block = m_current_chunk->getBlock(m_pos);
        float densite_fluide = block.getType() == Block::Type::Air ? 1.f : 0.f;
        if (densite_fluide > 0.f) {
            forces.push_back(densite_fluide * -forces[0] * m_volume / (m_weight / m_volume));
            forces.push_back(densite_fluide * m_vel * -m_drag);
        }

        RigidBody::update(_deltaTime, forces);

        // Chunk collision detection
        // cube_body.bounce(static_friction, applyTransformation(triangle_normal, 0.f, terrain_node.m_transfo.computeTransformationMatrix()));
    }

    void initShaderData() {
        for (size_t i = 0; i < m_hitbox.size(); i++) {
            m_hitbox[i].initShaderData();
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