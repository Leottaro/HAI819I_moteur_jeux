#pragma once

// USUAL INCLUDES
#include "objects/blocks.hpp"
#include "objects/textures.hpp"

constexpr std::array<int, 6> OPPOSITE_FACE{3, 4, 5, 0, 1, 2};

class Block {
public:
    static constexpr glm::ivec3 posToBlockPos(const glm::vec3& _pos) {
        return glm::ivec3(std::floor(_pos.x), std::floor(_pos.y), std::floor(_pos.z));
    }

    struct FaceData {
        std::array<glm::u8vec3, 4> vertices;
        glm::i8vec3 normal;
        glm::i8vec3 tangent;
        glm::i8vec3 bitangent;
        std::array<glm::uvec3, 2> triangles;
    };
    static constexpr std::array<FaceData, 6> FACE_DATA = {{
        {{{{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0}}}, {0, 0, -1}, {1, 0, 0}, {0, 1, 0}, {{{0u, 2u, 1u}, {0u, 3u, 2u}}}},  // Front (-Z)
        {{{{0, 0, 1}, {0, 0, 0}, {0, 1, 0}, {0, 1, 1}}}, {-1, 0, 0}, {0, 0, -1}, {0, 1, 0}, {{{0u, 2u, 1u}, {0u, 3u, 2u}}}}, // Left  (-X)
        {{{{0, 0, 1}, {1, 0, 1}, {1, 0, 0}, {0, 0, 0}}}, {0, -1, 0}, {1, 0, 0}, {0, 0, -1}, {{{0u, 2u, 1u}, {0u, 3u, 2u}}}}, // Bottom(-Y)
        {{{{1, 0, 1}, {0, 0, 1}, {0, 1, 1}, {1, 1, 1}}}, {0, 0, 1}, {-1, 0, 0}, {0, 1, 0}, {{{0u, 2u, 1u}, {0u, 3u, 2u}}}},  // Back  (+Z)
        {{{{1, 0, 0}, {1, 0, 1}, {1, 1, 1}, {1, 1, 0}}}, {1, 0, 0}, {0, 0, 1}, {0, 1, 0}, {{{0u, 2u, 1u}, {0u, 3u, 2u}}}},   // Right (+X)
        {{{{0, 1, 0}, {1, 1, 0}, {1, 1, 1}, {0, 1, 1}}}, {0, 1, 0}, {1, 0, 0}, {0, 0, 1}, {{{0u, 2u, 1u}, {0u, 3u, 2u}}}},   // Top   (+Y)
    }};

    static constexpr std::array<glm::ivec3, 6> NEIGHBOURS_POS{
        glm::ivec3(0, 0, -1), // Front (-Z)
        glm::ivec3(-1, 0, 0), // Left  (-X)
        glm::ivec3(0, -1, 0), // Bottom(-Y)
        glm::ivec3(0, 0, 1),  // Back  (+Z)
        glm::ivec3(1, 0, 0),  // Right (+X)
        glm::ivec3(0, 1, 0),  // Top   (+Y)
    };

private:
    BlockType m_type;
    glm::ivec3 m_pos;

public:
    std::array<Block*, 6> m_neighbours{nullptr};

    Block(Block&& other) noexcept : m_type(other.m_type), m_pos(other.m_pos), m_neighbours(other.m_neighbours) {
        other.m_neighbours.fill(nullptr);
        for (uint i = 0; i < 6; i++)
            if (m_neighbours[i] != nullptr)
                m_neighbours[i]->m_neighbours[OPPOSITE_FACE[i]] = this;
    }
    Block& operator=(Block&& other) noexcept {
        if (this == &other)
            return *this;

        m_type = other.m_type;
        m_pos = other.m_pos;
        m_neighbours = other.m_neighbours;
        other.m_neighbours.fill(nullptr);

        for (uint i = 0; i < 6; i++)
            if (m_neighbours[i] != nullptr)
                m_neighbours[i]->m_neighbours[OPPOSITE_FACE[i]] = this;

        return *this;
    }
    Block(const Block& other) : m_type(other.m_type), m_pos(other.m_pos), m_neighbours(other.m_neighbours) {
        // other.m_neighbours.fill(nullptr);
        for (uint i = 0; i < 6; i++)
            if (m_neighbours[i] != nullptr)
                m_neighbours[i]->m_neighbours[OPPOSITE_FACE[i]] = this;
    }
    Block& operator=(const Block& other) {
        if (this == &other)
            return *this;

        m_type = other.m_type;
        m_pos = other.m_pos;
        m_neighbours = other.m_neighbours;

        // other.m_neighbours.fill(nullptr);
        for (uint i = 0; i < 6; i++)
            if (m_neighbours[i] != nullptr)
                m_neighbours[i]->m_neighbours[OPPOSITE_FACE[i]] = this;

        return *this;
    }

    ~Block() = default;

    Block() : m_type(BlockType::Air) {}
    Block(BlockType _type, const glm::ivec3& _pos) : m_type(_type), m_pos(_pos) {}

    inline const BlockType& getType() const { return m_type; }
    inline BlockType& getType() { return m_type; }
    inline const glm::ivec3& getPos() const { return m_pos; }
    inline glm::ivec3& getPos() { return m_pos; }
    inline Block shallowCopy() const { return Block(m_type, m_pos); }

    constexpr float getFriction() const { return getBlockTypeData(m_type).friction; }
    constexpr float getRestitution() const { return getBlockTypeData(m_type).restitution; }
    constexpr float getStaticFriction() const { return getBlockTypeData(m_type).static_friction; }
    constexpr float getDensity() const { return getBlockTypeData(m_type).fluid_density; }
    constexpr bool hasHitbox() const { return getBlockTypeData(m_type).has_hitbox; }
    constexpr BlockTransparency getTransparence() const { return getBlockTypeData(m_type).transparence; }
    constexpr BlockTexture getTexture(int face_i) const { return getBlockTypeTexture(m_type, face_i); }

    inline bool update(int face_i) {
        if (m_type == BlockType::Grass && face_i == 5 && m_neighbours[face_i]->getTransparence() == BlockTransparency::SOLID) { // grass and +Z
            m_type = BlockType::Dirt;
            return true;
        }

        return false;
    }
};