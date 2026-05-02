#pragma once

#define GLM_FORCE_CONSTEXPR
#include <array>
#include <cstddef>
#include <glm/glm.hpp>

// FACE

struct FaceData {
    std::array<glm::u8vec3, 4> vertices;
    glm::vec3 normal;
    std::array<glm::uvec3, 2> triangles;
};

// BLOCKS

enum class BlockType : size_t {
    Air = 0,
    Stone,
    Dirt,
    Grass,
    Glass,
    NUMBER_OF_TYPES
};
static constexpr size_t BLOCK_TYPES_N = static_cast<size_t>(BlockType::NUMBER_OF_TYPES);
static constexpr std::array<std::array<std::array<float, 2>, 6>, BLOCK_TYPES_N - 1> UB_TABLE_DATA = {{
    // Front (-Z)    Left  (-X)    Bottom(-Y)    Back  (+Z)    Right (+X)    Top   (+Y)
    {{{{0.f, 0.f}}, {{0.f, 0.f}}, {{0.f, 0.f}}, {{0.f, 0.f}}, {{0.f, 0.f}}, {{0.f, 0.f}}}},  // Stone
    {{{{1.f, 0.f}}, {{1.f, 0.f}}, {{1.f, 0.f}}, {{1.f, 0.f}}, {{1.f, 0.f}}, {{1.f, 0.f}}}},  // Dirt
    {{{{2.f, 0.f}}, {{2.f, 0.f}}, {{1.f, 0.f}}, {{2.f, 0.f}}, {{2.f, 0.f}}, {{3.f, 0.f}}}},  // Grass
    {{{{0.f, 1.f}}, {{0.f, 1.f}}, {{0.f, 1.f}}, {{0.f, 1.f}}, {{0.f, 1.f}}, {{0.f, 1.f}}}}   // Glass
}};

static constexpr std::array<FaceData, 6> FACE_DATA = {{
    {{{{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0}}}, {0.f, 0.f, -1.f}, {{{0u, 2u, 1u}, {0u, 3u, 2u}}}},  // Front (-Z)
    {{{{0, 0, 1}, {0, 0, 0}, {0, 1, 0}, {0, 1, 1}}}, {-1.f, 0.f, 0.f}, {{{0u, 2u, 1u}, {0u, 3u, 2u}}}},  // Left  (-X)
    {{{{0, 0, 1}, {1, 0, 1}, {1, 0, 0}, {0, 0, 0}}}, {0.f, -1.f, 0.f}, {{{0u, 2u, 1u}, {0u, 3u, 2u}}}},  // Bottom(-Y)
    {{{{1, 0, 1}, {0, 0, 1}, {0, 1, 1}, {1, 1, 1}}}, {0.f, 0.f, 1.f}, {{{0u, 2u, 1u}, {0u, 3u, 2u}}}},   // Back  (+Z)
    {{{{1, 0, 0}, {1, 0, 1}, {1, 1, 1}, {1, 1, 0}}}, {1.f, 0.f, 0.f}, {{{0u, 2u, 1u}, {0u, 3u, 2u}}}},   // Right (+X)
    {{{{0, 1, 0}, {1, 1, 0}, {1, 1, 1}, {0, 1, 1}}}, {0.f, 1.f, 0.f}, {{{0u, 2u, 1u}, {0u, 3u, 2u}}}},   // Top   (+Y)
}};
static constexpr std::array<glm::ivec3, 6> NEIGHBOURS_POS{
    glm::ivec3(0, 0, -1),  // Front (-Z)
    glm::ivec3(-1, 0, 0),  // Left  (-X)
    glm::ivec3(0, -1, 0),  // Bottom(-Y)
    glm::ivec3(0, 0, 1),   // Back  (+Z)
    glm::ivec3(1, 0, 0),   // Right (+X)
    glm::ivec3(0, 1, 0),   // Top   (+Y)
};