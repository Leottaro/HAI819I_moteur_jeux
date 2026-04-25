#pragma once

// GLM
#define GLM_FORCE_CONSTEXPR
#include <glm/glm.hpp>

// USUAL INCLUDES

constexpr std::array<int, 6> OPPOSITE_FACE{3, 4, 5, 0, 1, 2};

class Block {
public:
    // FACE

    struct FaceData {
        std::array<glm::u8vec3, 4> vertices;
        glm::i8vec3 normal;
        std::array<glm::uvec3, 2> triangles;
    };
    static constexpr std::array<FaceData, 6> FACE_DATA = {{
        {{{{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0}}}, {0, 0, -1}, {{{0u, 2u, 1u}, {0u, 3u, 2u}}}}, // Front (-Z)
        {{{{0, 0, 1}, {0, 0, 0}, {0, 1, 0}, {0, 1, 1}}}, {-1, 0, 0}, {{{0u, 2u, 1u}, {0u, 3u, 2u}}}}, // Left  (-X)
        {{{{0, 0, 1}, {1, 0, 1}, {1, 0, 0}, {0, 0, 0}}}, {0, -1, 0}, {{{0u, 2u, 1u}, {0u, 3u, 2u}}}}, // Bottom(-Y)
        {{{{1, 0, 1}, {0, 0, 1}, {0, 1, 1}, {1, 1, 1}}}, {0, 0, 1}, {{{0u, 2u, 1u}, {0u, 3u, 2u}}}},  // Back  (+Z)
        {{{{1, 0, 0}, {1, 0, 1}, {1, 1, 1}, {1, 1, 0}}}, {1, 0, 0}, {{{0u, 2u, 1u}, {0u, 3u, 2u}}}},  // Right (+X)
        {{{{0, 1, 0}, {1, 1, 0}, {1, 1, 1}, {0, 1, 1}}}, {0, 1, 0}, {{{0u, 2u, 1u}, {0u, 3u, 2u}}}},  // Top   (+Y)
    }};
    static constexpr std::array<glm::ivec3, 6> NEIGHBOURS_POS{
        glm::ivec3(0, 0, -1), // Front (-Z)
        glm::ivec3(-1, 0, 0), // Left  (-X)
        glm::ivec3(0, -1, 0), // Bottom(-Y)
        glm::ivec3(0, 0, 1),  // Back  (+Z)
        glm::ivec3(1, 0, 0),  // Right (+X)
        glm::ivec3(0, 1, 0),  // Top   (+Y)
    };

    // BLOCKS

    enum class Type { Air,
                      Stone,
                      Dirt,
                      Grass,
                      Glass,
                      IronBlock,
                      RedstoneLamp,
                      DiamondOre,
                      NUMBER_OF_TYPES
    };
    static constexpr size_t BLOCK_TYPES_N = static_cast<size_t>(Type::NUMBER_OF_TYPES);
    static constexpr std::array<std::array<std::array<float, 2>, 6>, BLOCK_TYPES_N - 1> UB_TABLE_DATA = {{
        // Front (-Z)    Left  (-X)    Bottom(-Y)    Back  (+Z)    Right (+X)    Top   (+Y)
        {{{{0.f, 0.f}}, {{0.f, 0.f}}, {{0.f, 0.f}}, {{0.f, 0.f}}, {{0.f, 0.f}}, {{0.f, 0.f}}}}, // Stone
        {{{{1.f, 0.f}}, {{1.f, 0.f}}, {{1.f, 0.f}}, {{1.f, 0.f}}, {{1.f, 0.f}}, {{1.f, 0.f}}}}, // Dirt
        {{{{2.f, 0.f}}, {{2.f, 0.f}}, {{1.f, 0.f}}, {{2.f, 0.f}}, {{2.f, 0.f}}, {{3.f, 0.f}}}}, // Grass
        {{{{0.f, 1.f}}, {{0.f, 1.f}}, {{0.f, 1.f}}, {{0.f, 1.f}}, {{0.f, 1.f}}, {{0.f, 1.f}}}}, // Glass
        {{{{1.f, 1.f}}, {{1.f, 1.f}}, {{1.f, 1.f}}, {{1.f, 1.f}}, {{1.f, 1.f}}, {{1.f, 1.f}}}}, // IronBlock
        {{{{2.f, 1.f}}, {{2.f, 1.f}}, {{2.f, 1.f}}, {{2.f, 1.f}}, {{2.f, 1.f}}, {{2.f, 1.f}}}}, // RedstoneLamp
        {{{{3.f, 1.f}}, {{3.f, 1.f}}, {{3.f, 1.f}}, {{3.f, 1.f}}, {{3.f, 1.f}}, {{3.f, 1.f}}}}  // DiamondOre
    }};
    static constexpr std::array<std::array<float, 2>, BLOCK_TYPES_N> PHYSICS_TABLE = {{
        // friction, bounciness
        {0.f, 0.f}, // Air
        {0.f, 1.f}, // Stone
        {0.f, 1.f}, // Dirt
        {0.f, 1.f}, // Grass
        {0.f, 1.f}, // Glass
        {0.f, 1.f}, // IronBlock
        {0.f, 1.f}, // RedstoneLamp
        {0.f, 1.f}  // DiamondOre
    }};

    // TEXTURES

    static constexpr float ATLAS_SIZE = 4; // in blocks
    static constexpr std::array<glm::vec2, 4> getUV(Type type, int face) {
        const auto &uv = UB_TABLE_DATA[static_cast<size_t>(type) - 1][face];
        return {
            glm::vec2(uv[0] + 1.f, uv[1] + 1.f) / ATLAS_SIZE,
            glm::vec2(uv[0], uv[1] + 1.f) / ATLAS_SIZE,
            glm::vec2(uv[0], uv[1]) / ATLAS_SIZE,
            glm::vec2(uv[0] + 1.f, uv[1]) / ATLAS_SIZE,
        };
    }

private:
    Type m_type;
    glm::ivec3 m_pos;

public:
    std::array<Block *, 6> m_neighbours{nullptr};

    Block() : m_type(Type::Air) {}
    Block(Type _type, const glm::ivec3 &_pos) : m_type(_type), m_pos(_pos) {}

    inline const Type &getType() const { return m_type; }
    inline Type &getType() { return m_type; }
    inline const glm::ivec3 &getPos() const { return m_pos; }
    inline glm::ivec3 &getPos() { return m_pos; }

    inline bool isTransparent() const { return m_type == Type::Air || m_type == Type::Glass; }
    inline float getDensity() const {
        switch (m_type) {
        case Type::Air:
            return 1.f;
        // case Type::Water:
        //     return 1000.f;
        // case Type::Lava:
        //     return 2000.f;
        default:
            return 0.f;
        }
    }
    inline bool hasHitbox() const { return !(m_type == Type::Air); }
    inline const std::array<float, 2> &getCollisionStats() const { return PHYSICS_TABLE[size_t(m_type)]; }
};