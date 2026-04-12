#pragma once

// GLM
#include <glm/glm.hpp>

// USUAL INCLUDES

constexpr std::array<int, 6> OPPOSITE_FACE{3, 4, 5, 0, 1, 2};

class Block {
public:
    enum class BlockType { Air,
                           Stone,
                           Dirt,
                           Grass,
                           Bedrock };

    static constexpr std::array<glm::ivec3, 6> NEIGHBOURS_POS{
        glm::ivec3(0, 0, -1), // Front (-Z)
        glm::ivec3(-1, 0, 0), // Left  (-X)
        glm::ivec3(0, -1, 0), // Bottom(-Y)
        glm::ivec3(0, 0, 1),  // Back  (+Z)
        glm::ivec3(1, 0, 0),  // Right (+X)
        glm::ivec3(0, 1, 0),  // Top   (+Y)
    };

    struct FaceData {
        std::array<glm::vec3, 4> vertices;
        glm::vec3 normal;
        std::array<glm::uvec3, 2> triangles;
    };
    static constexpr std::array<FaceData, 6> FACE_DATA = {{
        {{{{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0}}}, {0.f, 0.f, -1.f}, {{{0u, 2u, 1u}, {0u, 3u, 2u}}}}, // Front (-Z)
        {{{{0, 0, 1}, {0, 0, 0}, {0, 1, 0}, {0, 1, 1}}}, {-1.f, 0.f, 0.f}, {{{0u, 2u, 1u}, {0u, 3u, 2u}}}}, // Left  (-X)
        {{{{0, 0, 1}, {1, 0, 1}, {1, 0, 0}, {0, 0, 0}}}, {0.f, -1.f, 0.f}, {{{0u, 2u, 1u}, {0u, 3u, 2u}}}}, // Bottom(-Y)
        {{{{1, 0, 1}, {0, 0, 1}, {0, 1, 1}, {1, 1, 1}}}, {0.f, 0.f, 1.f}, {{{0u, 2u, 1u}, {0u, 3u, 2u}}}},  // Back  (+Z)
        {{{{1, 0, 0}, {1, 0, 1}, {1, 1, 1}, {1, 1, 0}}}, {1.f, 0.f, 0.f}, {{{0u, 2u, 1u}, {0u, 3u, 2u}}}},  // Right (+X)
        {{{{0, 1, 0}, {1, 1, 0}, {1, 1, 1}, {0, 1, 1}}}, {0.f, 1.f, 0.f}, {{{0u, 2u, 1u}, {0u, 3u, 2u}}}},  // Top   (+Y)
    }};

private:
    BlockType m_type;
    glm::ivec3 m_pos;

public:
    std::array<Block *, 6> m_neighbours{nullptr};

    Block() : m_type(BlockType::Air) {}
    Block(BlockType _type, const glm::ivec3 &_pos) : m_type(_type), m_pos(_pos) {}

    inline const BlockType &getType() const { return m_type; }
    inline BlockType &getType() { return m_type; }
    inline const glm::ivec3 &getPos() const { return m_pos; }
    inline glm::ivec3 &getPos() { return m_pos; }
};