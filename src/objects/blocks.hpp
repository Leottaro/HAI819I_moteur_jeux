#pragma once

#define GLM_FORCE_CONSTEXPR
#include <glm/glm.hpp>

#include <array>
#include <cstddef>

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

constexpr std::array<std::array<glm::vec2, 6>, BLOCK_TYPES_N - 1> UV_TABLE_DATA = {{
    // Front (-Z)    Left  (-X)    Bottom(-Y)    Back  (+Z)    Right (+X)    Top   (+Y)
    {{{0.f, 0.f}, {0.f, 0.f}, {0.f, 0.f}, {0.f, 0.f}, {0.f, 0.f}, {0.f, 0.f}}}, // Stone
    {{{1.f, 0.f}, {1.f, 0.f}, {1.f, 0.f}, {1.f, 0.f}, {1.f, 0.f}, {1.f, 0.f}}}, // Dirt
    {{{2.f, 0.f}, {2.f, 0.f}, {1.f, 0.f}, {2.f, 0.f}, {2.f, 0.f}, {3.f, 0.f}}}, // Grass
    {{{0.f, 1.f}, {0.f, 1.f}, {0.f, 1.f}, {0.f, 1.f}, {0.f, 1.f}, {0.f, 1.f}}}, // Glass
    {{{1.f, 1.f}, {1.f, 1.f}, {1.f, 1.f}, {1.f, 1.f}, {1.f, 1.f}, {1.f, 1.f}}}, // IronBlock
    {{{2.f, 1.f}, {2.f, 1.f}, {2.f, 1.f}, {2.f, 1.f}, {2.f, 1.f}, {2.f, 1.f}}}, // RedstoneLamp
    {{{3.f, 1.f}, {3.f, 1.f}, {3.f, 1.f}, {3.f, 1.f}, {3.f, 1.f}, {3.f, 1.f}}}, // DiamondOre
    {{{0.f, 2.f}, {0.f, 2.f}, {0.f, 2.f}, {0.f, 2.f}, {0.f, 2.f}, {0.f, 2.f}}}, // SlimeBlock
}};

enum class BlockTransparence : size_t {
    SOLID = 0,
    TRANSPARENT,
    TRANSLUCENT
};
struct BlockTypeData {
    float friction;                 // Describes the friction of a bounce
    float restitution;              // Describes the restitution of a bounce
    float static_friction;          // Describes the friction when standing on it
    float fluid_density;            // Describes the block's density when in it
    bool has_hitbox;                // Describes if the block has an hitbox
    BlockTransparence transparence; // Describes the block's transparence // TODO: dans l'atlas map ?
};

constexpr std::array<BlockTypeData, BLOCK_TYPES_N> BLOCK_TYPE_DATA{{
    {0.f, 0.f, 0.f, 1.f, false, BlockTransparence::TRANSPARENT},              // Air
    {1.f / 2.f, 0.f, 1.f / 3.f, 0.f, true, BlockTransparence::SOLID},         // Stone
    {1.f / 2.f, 0.f, 1.f / 3.f, 0.f, true, BlockTransparence::SOLID},         // Dirt
    {1.f / 2.f, 0.f, 1.f / 3.f, 0.f, true, BlockTransparence::SOLID},         // Grass
    {1.f / 2.f, 0.f, 1.f / 3.f, 0.f, true, BlockTransparence::TRANSPARENT},   // Glass
    {1.f / 2.f, 0.f, 1.f / 3.f, 0.f, true, BlockTransparence::SOLID},         // IronBlock
    {1.f / 2.f, 0.f, 1.f / 3.f, 0.f, true, BlockTransparence::SOLID},         // RedstoneLamp
    {1.f / 2.f, 0.f, 1.f / 3.f, 0.f, true, BlockTransparence::SOLID},         // DiamondOre
    {1.f / 2.f, 0.75f, 1.f / 3.f, 0.f, true, BlockTransparence::TRANSLUCENT}, // SlimeBlock
}};

constexpr const BlockTypeData& getBlockTypeData(BlockType type) { return BLOCK_TYPE_DATA[static_cast<size_t>(type)]; }