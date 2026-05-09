#pragma once

#define GLM_FORCE_CONSTEXPR
#include <array>
#include <cstddef>
#include <glm/glm.hpp>

enum class BlockType : size_t {
    Air = 0,
    Stone,
    Dirt,
    Grass,
    Glass,
    IronBlock,
    RedstoneLamp,
    DiamondOre,
    SlimeBlock,
    __NUMBER_OF_TYPES
};
constexpr size_t BLOCK_TYPES_N = static_cast<size_t>(BlockType::__NUMBER_OF_TYPES);

constexpr std::array<std::array<std::array<float, 2>, 6>, BLOCK_TYPES_N - 1> UB_TABLE_DATA = {{
    // Front (-Z)    Left  (-X)    Bottom(-Y)    Back  (+Z)    Right (+X)    Top   (+Y)
    {{{{0.f, 0.f}}, {{0.f, 0.f}}, {{0.f, 0.f}}, {{0.f, 0.f}}, {{0.f, 0.f}}, {{0.f, 0.f}}}},  // Stone
    {{{{1.f, 0.f}}, {{1.f, 0.f}}, {{1.f, 0.f}}, {{1.f, 0.f}}, {{1.f, 0.f}}, {{1.f, 0.f}}}},  // Dirt
    {{{{2.f, 0.f}}, {{2.f, 0.f}}, {{1.f, 0.f}}, {{2.f, 0.f}}, {{2.f, 0.f}}, {{3.f, 0.f}}}},  // Grass
    {{{{0.f, 1.f}}, {{0.f, 1.f}}, {{0.f, 1.f}}, {{0.f, 1.f}}, {{0.f, 1.f}}, {{0.f, 1.f}}}},  // Glass
    {{{{1.f, 1.f}}, {{1.f, 1.f}}, {{1.f, 1.f}}, {{1.f, 1.f}}, {{1.f, 1.f}}, {{1.f, 1.f}}}},  // IronBlock
    {{{{2.f, 1.f}}, {{2.f, 1.f}}, {{2.f, 1.f}}, {{2.f, 1.f}}, {{2.f, 1.f}}, {{2.f, 1.f}}}},  // RedstoneLamp
    {{{{3.f, 1.f}}, {{3.f, 1.f}}, {{3.f, 1.f}}, {{3.f, 1.f}}, {{3.f, 1.f}}, {{3.f, 1.f}}}},  // DiamondOre
    {{{{0.f, 2.f}}, {{0.f, 2.f}}, {{0.f, 2.f}}, {{0.f, 2.f}}, {{0.f, 2.f}}, {{0.f, 2.f}}}},  // SlimeBlock
}};

constexpr std::array<bool, static_cast<size_t>(BlockType::__NUMBER_OF_TYPES)> IS_TRANSPARENT{
    true,
    false,
    false,
    false,
    true,
    false,
    false,
    false,
    true};

constexpr std::array<std::array<float, 2>, BLOCK_TYPES_N> PHYSICS_TABLE = {{
    // friction, bounciness
    {0.f, 0.f},   // Air
    {0.5f, 0.f},  // Stone
    {0.5f, 0.f},  // Dirt
    {0.5f, 0.f},  // Grass
    {0.5f, 0.f},  // Glass
    {0.5f, 0.f},  // IronBlock
    {0.5f, 0.f},  // RedstoneLamp
    {0.5f, 0.f},  // DiamondOre
    {0.5f, 1.f}   // SlimeBlock
}};