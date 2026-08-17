#pragma once

#define GLM_FORCE_CONSTEXPR
#include <array>
#include <cstddef>
#include <glm/glm.hpp>

#include "src/objects/textures.hpp"

enum class BlockType : uint8_t {
    Air = 0,
    PierreDeLit,  // On ne plagie toujours pas minecraft !
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
    HoneyBlock,
    Ice,
    Water,
    __NUMBER_OF_TYPES
};
constexpr uint8_t BLOCK_TYPES_N = static_cast<uint8_t>(BlockType::__NUMBER_OF_TYPES);

constexpr std::array<std::string_view, BLOCK_TYPES_N> BLOCK_NAMES = {{
    "Air",
    "Pierre de Lit",
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
}};

constexpr std::string_view getBlockTypeName(BlockType block_type) { return BLOCK_NAMES[static_cast<uint8_t>(block_type)]; }

// for IMGUI
constexpr auto buildBlockNamesString() {
    std::array<char, 256> result = {};
    size_t pos = 0;
    for (const auto& name : BLOCK_NAMES) {
        for (char c : name) {
            result[pos++] = c;
        }
        result[pos++] = '\0';
    }
    return result;
}
constexpr auto BLOCK_NAMES_STRING = buildBlockNamesString();

using Te = BlockTexture;  // Pour pas que les lignes en dessous fassent 3.5km

// si on met pas le clang off ça massacre sans aucun scrupules cette belle indentation
// clang-format off
constexpr std::array<std::array<Te, 6>, BLOCK_TYPES_N - 1> TEXTURE_TABLE_DATA = {{
 //    Front (-Z)                 Left (-X)                  Bottom (-Y)                  Back (+Z)                  Right (+X)                Top (+Y)
 {{Te::pierre_de_lit,    Te::pierre_de_lit,    Te::pierre_de_lit,      Te::pierre_de_lit,    Te::pierre_de_lit,   Te::pierre_de_lit}},   // PierreDeLit
    {{Te::stone,            Te::stone,            Te::stone,              Te::stone,            Te::stone,           Te::stone}},           // Stone
    {{Te::dirt,             Te::dirt,             Te::dirt,               Te::dirt,             Te::dirt,            Te::dirt}},            // Dirt
    {{Te::grass_side,       Te::grass_side,       Te::dirt,               Te::grass_side,       Te::grass_side,      Te::grass_top}},       // Grass
    {{Te::glass,            Te::glass,            Te::glass,              Te::glass,            Te::glass,           Te::glass}},           // Glass
    {{Te::iron_block,       Te::iron_block,       Te::iron_block,         Te::iron_block,       Te::iron_block,      Te::iron_block}},      // IronBlock
    {{Te::redstone_lamp,    Te::redstone_lamp,    Te::redstone_lamp,      Te::redstone_lamp,    Te::redstone_lamp,   Te::redstone_lamp}},   // RedstoneLamp
    {{Te::diamond_ore,      Te::diamond_ore,      Te::diamond_ore,        Te::diamond_ore,      Te::diamond_ore,     Te::diamond_ore}},     // DiamondOre
    {{Te::slime_block,      Te::slime_block,      Te::slime_block,        Te::slime_block,      Te::slime_block,     Te::slime_block}},     // SlimeBlock
    {{Te::oak_log_side,     Te::oak_log_side,     Te::oak_log_side,       Te::oak_log_side,     Te::oak_log_side,    Te::oak_log_top}},     // OakLog
    {{Te::oak_leaves,       Te::oak_leaves,       Te::oak_leaves,         Te::oak_leaves,       Te::oak_leaves,      Te::oak_leaves}},      // OakLeaves
    {{Te::honey_block_side, Te::honey_block_side, Te::honey_block_bottom, Te::honey_block_side, Te::honey_block_side,Te::honey_block_top}}, // Honey Block
    {{Te::ice,              Te::ice,              Te::ice,                Te::ice,              Te::ice,             Te::ice}},             // Ice
    {{Te::water,            Te::water,            Te::water,              Te::water,            Te::water,           Te::water}},           // Water
}};
constexpr Te getBlockTypeTexture(BlockType block_type, uint8_t face_i) { return TEXTURE_TABLE_DATA[static_cast<uint8_t>(block_type)-1][face_i]; }

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
    {0.5f, 0.f, 0.333f, 0.f, true, BlockTransparency::SOLID},           // Pierre de Lit
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
    {0.2f, 0.f, 0.1f, 0.f, true, BlockTransparency::TRANSLUCENT},      // Ice
    {0.2f, 0.f, 0.05f, 1250.f, false, BlockTransparency::TRANSLUCENT},      // Water
}};
constexpr const BlockTypeData& getBlockTypeData(BlockType type) { return BLOCK_TYPE_DATA[static_cast<uint8_t>(type)]; }

// https://stackoverflow.com/questions/16388510/evaluate-a-string-with-a-switch-in-c
constexpr unsigned int str2int(const std::string& str, int h = 0)
{
    return !str[h] ? 5381 : (str2int(str, h+1) * 33) ^ str[h];
}

constexpr const BlockType minecraftTranslation(const std::string& mc_id) {
    switch (str2int(mc_id))
    {
    case str2int("minecraft:grass_block"):
        return BlockType::Grass;
    
    case str2int("minecraft:dirt"):
    case str2int("minecraft:coarse_dirt"):
    case str2int("minecraft:dirt_path"):
    case str2int("minecraft:rooted_dirt"):
        return BlockType::Dirt;
    
    case str2int("minecraft:stone"):
    case str2int("minecraft:deepslate"):
        return BlockType::Stone;
    
    case str2int("minecraft:glass"):
    case str2int("minecraft:glass_pane"):
        return BlockType::Glass;
    
    case str2int("minecraft:oak_log"):
        return BlockType::OakLog;
    
    default:
        return BlockType::PierreDeLit;
    }
}