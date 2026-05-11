#pragma once

// GLM
#define GLM_FORCE_CONSTEXPR
#include <glm/glm.hpp>

// USUAL INCLUDES
#include "objects/blocks.hpp"
#include <functional>
#include <set>
#include <map>

constexpr std::array<int, 6> OPPOSITE_FACE{3, 4, 5, 0, 1, 2};

template <typename T, size_t n>
struct glmVecLexicoGraphic {
    bool operator()(const glm::vec<n, T, glm::packed_highp>& a, const glm::vec<n, T, glm::packed_highp>& b) const {
        return a.x != b.x   ? a.x < b.x
               : a.y != b.y ? a.y < b.y
                            : a.z < b.z;
    }
};

template <typename T, size_t n>
using VecSet = std::set<glm::vec<n, T, glm::packed_highp>, glmVecLexicoGraphic<T, n>>;
template <typename T, size_t n, typename V>
using VecMap = std::map<glm::vec<n, T, glm::packed_highp>, V, glmVecLexicoGraphic<T, n>>;

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

    static constexpr std::array<glm::vec2, 4> getUV(float atlas_size, BlockType block_type, int face_i) {
        size_t type_idx = static_cast<size_t>(block_type) - 1;
        const auto& uv = UB_TABLE_DATA[type_idx][face_i];
        return {
            glm::vec2(uv[0], uv[1] + 1.f) / atlas_size,
            glm::vec2(uv[0] + 1.f, uv[1] + 1.f) / atlas_size,
            glm::vec2(uv[0] + 1.f, uv[1]) / atlas_size,
            glm::vec2(uv[0], uv[1]) / atlas_size,
        };
    }

private:
    BlockType m_type;
    glm::ivec3 m_pos;

public:
    std::array<Block*, 6> m_neighbours{nullptr};

    Block(Block&&) = delete;
    Block(const Block&) = delete;
    Block& operator=(const Block&) = delete;
    Block& operator=(Block&&) = delete;
    Block() : m_type(BlockType::Air) {}
    Block(BlockType _type, const glm::ivec3& _pos) : m_type(_type), m_pos(_pos) {}

    inline const BlockType& getType() const { return m_type; }
    inline BlockType& getType() { return m_type; }
    inline const glm::ivec3& getPos() const { return m_pos; }
    inline glm::ivec3& getPos() { return m_pos; }

    constexpr float getFriction() const { return getBlockTypeData(m_type).friction; }
    constexpr float getRestitution() const { return getBlockTypeData(m_type).restitution; }
    constexpr float getStaticFriction() const { return getBlockTypeData(m_type).static_friction; }
    constexpr float getDensity() const { return getBlockTypeData(m_type).fluid_density; }
    constexpr bool hasHitbox() const { return getBlockTypeData(m_type).has_hitbox; }
    constexpr BlockTransparence getTransparence() const { return getBlockTypeData(m_type).transparence; }
};