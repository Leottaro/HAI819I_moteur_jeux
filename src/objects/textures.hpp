#pragma once

#include <array>
#include <cmath>
#include <cstddef>
#include <string_view>

enum class BlockTexture : uint8_t {
    stone,
    dirt,
    grass_side,
    grass_top,
    glass,
    iron_block,
    redstone_lamp,
    diamond_ore,
    slime_block,
    oak_log_side,
    oak_log_top,
    oak_leaves,
    honey_block_top,
    honey_block_side,
    honey_block_bottom,
    ice,
    water,
    pierre_de_lit, // Changement de nom parce qu'on plagie pas minecraft ici !
    NUMBER_OF_TEXTURES
};

constexpr uint8_t TEXTURE_NUMBER = static_cast<uint8_t>(BlockTexture::NUMBER_OF_TEXTURES);
constexpr unsigned firstSquare(unsigned x, unsigned dims = 0) {
    return dims * dims < x ? firstSquare(x, dims + 1) : dims;
}
constexpr uint32_t ATLAS_DIMS = firstSquare(TEXTURE_NUMBER);

struct BlockTextureData {
    std::string_view name;
    bool can_rotate;
    bool can_flip_x;
    bool can_flip_y;
};

constexpr std::array<BlockTextureData, TEXTURE_NUMBER> BLOCK_TEXTURE_DATA{{
    // NAME CAN_ROTATE  CAN_FLIP_X  CAN_FLIP_Y
    {"stone", true, true, true},
    {"dirt", true, true, true},
    {"grass_side", false, true, false},
    {"grass_top", true, true, true},
    {"glass", false, false, false},
    {"iron_block", true, true, true},
    {"redstone_lamp", true, true, true},
    {"diamond_ore", true, true, true},
    {"slime_block", true, true, true},
    {"oak_log_side", false, true, true},
    {"oak_log_top", true, true, true},
    {"oak_leaves", true, true, true},
    {"honey_block_top", true, true, true},
    {"honey_block_side", true, true, true},
    {"honey_block_bottom", true, true, true},
    {"ice", true, true, true},
    {"water", true, true, true},
    {"pierre_de_lit", true, true, true},
}};
constexpr const BlockTextureData& getBlockTextureData(BlockTexture texture) { return BLOCK_TEXTURE_DATA[static_cast<uint8_t>(texture)]; }