#pragma once

#define GLM_FORCE_CONSTEXPR
#include <array>
#include <cstddef>
#include <glm/glm.hpp>

#include "src/objects/textures.hpp"

enum class BlockType : uint8_t {
    Air = 0,
    Stone,
    Dirt,
    Grass,
    Glass,
    IronBlock,
    RedstoneLamp,
    DiamondOre,
    SlimeBlock,
    OakLog,
    OakLeaves,
    PierreDeLit,  // On ne plagie toujours pas minecraft !
    HoneyBlock,
    Ice,
    Water,
    __NUMBER_OF_TYPES
};
constexpr uint8_t BLOCK_TYPES_N = static_cast<uint8_t>(BlockType::__NUMBER_OF_TYPES);

constexpr std::array<std::string_view, BLOCK_TYPES_N> block_names = {{
    "air",
    "Stone",
    "Dirt",
    "Grass",
    "Glass",
    "Iron Block",
    "Redstone Lamp",
    "Diamond Ore",
    "Slime Block",
    "Oak Log",
    "Oak Leaves",
    "Honey Block",
    "Ice",
    "Water",
    "Pierre de Lit",
}};

using Te = Textures;  // Pour pas que les lignes en dessous fassent 3.5km

// si on met pas le clang off ça massacre sans aucun scrupules cette belle indentation
// clang-format off
constexpr std::array<std::array<glm::u32vec2, 6>, BLOCK_TYPES_N - 1> UV_TABLE_DATA = {{
 // Front (-Z)                  Left (-X)              Bottom (-Y)              Back (+Z)               Right (+X)              Top (+Y)
    {{TEX(Te::stone),            TEX(Te::stone),            TEX(Te::stone),              TEX(Te::stone),            TEX(Te::stone),           TEX(Te::stone)}},           // Stone
    {{TEX(Te::dirt),             TEX(Te::dirt),             TEX(Te::dirt),               TEX(Te::dirt),             TEX(Te::dirt),            TEX(Te::dirt)}},            // Dirt
    {{TEX(Te::grass_side),       TEX(Te::grass_side),       TEX(Te::dirt),               TEX(Te::grass_side),       TEX(Te::grass_side),      TEX(Te::grass_top)}},       // Grass
    {{TEX(Te::glass),            TEX(Te::glass),            TEX(Te::glass),              TEX(Te::glass),            TEX(Te::glass),           TEX(Te::glass)}},           // Glass
    {{TEX(Te::iron_block),       TEX(Te::iron_block),       TEX(Te::iron_block),         TEX(Te::iron_block),       TEX(Te::iron_block),      TEX(Te::iron_block)}},      // IronBlock
    {{TEX(Te::redstone_lamp),    TEX(Te::redstone_lamp),    TEX(Te::redstone_lamp),      TEX(Te::redstone_lamp),    TEX(Te::redstone_lamp),   TEX(Te::redstone_lamp)}},   // RedstoneLamp
    {{TEX(Te::diamond_ore),      TEX(Te::diamond_ore),      TEX(Te::diamond_ore),        TEX(Te::diamond_ore),      TEX(Te::diamond_ore),     TEX(Te::diamond_ore)}},     // DiamondOre
    {{TEX(Te::slime_block),      TEX(Te::slime_block),      TEX(Te::slime_block),        TEX(Te::slime_block),      TEX(Te::slime_block),     TEX(Te::slime_block)}},     // SlimeBlock
    {{TEX(Te::oak_log_side),     TEX(Te::oak_log_side),     TEX(Te::oak_log_side),       TEX(Te::oak_log_side),     TEX(Te::oak_log_side),    TEX(Te::oak_log_top)}},     // OakLog
    {{TEX(Te::oak_leaves),       TEX(Te::oak_leaves),       TEX(Te::oak_leaves),         TEX(Te::oak_leaves),       TEX(Te::oak_leaves),      TEX(Te::oak_leaves)}},      // OakLeaves
    {{TEX(Te::honey_block_side), TEX(Te::honey_block_side), TEX(Te::honey_block_bottom), TEX(Te::honey_block_side), TEX(Te::honey_block_side),TEX(Te::honey_block_top)}}, // Honey Block
    {{TEX(Te::ice),              TEX(Te::ice),              TEX(Te::ice),                TEX(Te::ice),              TEX(Te::ice),             TEX(Te::ice)}},             // Ice
    {{TEX(Te::water),            TEX(Te::water),            TEX(Te::water),              TEX(Te::water),            TEX(Te::water),           TEX(Te::water)}},           // Water
    {{TEX(Te::pierre_de_lit),    TEX(Te::pierre_de_lit),    TEX(Te::pierre_de_lit),      TEX(Te::pierre_de_lit),    TEX(Te::pierre_de_lit),   TEX(Te::pierre_de_lit)}},   // PierreDeLit
}};

enum class BlockTransparency : uint8_t {
    SOLID = 0,
    TRANSPARENT,
    SEMI_TRANSPARENT,
    TRANSLUCENT
};
struct BlockTypeData {
    float friction;                 // Describes the friction of a bounce
    float restitution;              // Describes the restitution of a bounce
    float static_friction;          // Describes the friction when standing on it
    float fluid_density;            // Describes the block's density when in it
    bool has_hitbox;                // Describes if the block has an hitbox
    BlockTransparency transparence; // Describes the block's transparence
};

constexpr std::array<BlockTypeData, BLOCK_TYPES_N> BLOCK_TYPE_DATA{{
    {0.f, 0.f, 0.f, 1.f, false, BlockTransparency::TRANSPARENT},        // Air
    {0.5f, 0.f, 0.333f, 0.f, true, BlockTransparency::SOLID},           // Stone
    {0.5f, 0.f, 0.333f, 0.f, true, BlockTransparency::SOLID},           // Dirt
    {0.5f, 0.f, 0.333f, 0.f, true, BlockTransparency::SOLID},           // Grass
    {0.5f, 0.f, 0.333f, 0.f, true, BlockTransparency::TRANSPARENT},     // Glass
    {0.5f, 0.f, 0.333f, 0.f, true, BlockTransparency::SOLID},           // IronBlock
    {0.5f, 0.f, 0.333f, 0.f, true, BlockTransparency::SOLID},           // RedstoneLamp
    {0.5f, 0.f, 0.333f, 0.f, true, BlockTransparency::SOLID},           // DiamondOre
    {0.5f, 0.75f, 0.333f, 0.f, true, BlockTransparency::TRANSLUCENT},   // SlimeBlock
    {0.5f, 0.f, 0.333f, 0.f, true, BlockTransparency::SOLID},           // Oak Log
    {0.5f, 0.f, 0.333f, 0.f, true, BlockTransparency::SEMI_TRANSPARENT},// Oak leaves
    {0.5f, 0.25f, 0.333f, 0.f, true, BlockTransparency::TRANSLUCENT},   // Honey Block
    {0.2f, 0.f, 0.05f, 0.f, true, BlockTransparency::TRANSLUCENT},      // Ice
    {0.2f, 0.f, 0.05f, 1000.f, false, BlockTransparency::TRANSLUCENT},      // Water
    {0.5f, 0.f, 0.333f, 0.f, true, BlockTransparency::SOLID},           // Pierre de Lit
}};

constexpr const BlockTypeData& getBlockTypeData(BlockType type) { return BLOCK_TYPE_DATA[static_cast<uint8_t>(type)]; }