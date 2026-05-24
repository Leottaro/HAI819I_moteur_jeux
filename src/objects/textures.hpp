#pragma once

#include <array>
#include <cmath>
#include <cstddef>
#include <string_view>

enum class Textures : uint64_t {
    air = 0,
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
    NUMBER_OF_TEXTURES
};

constexpr uint32_t ATLAS_DIMS = ceil(sqrt(static_cast<float>(Textures::NUMBER_OF_TEXTURES) - 1));

constexpr uint64_t TEXTURE_NUMBER = static_cast<size_t>(Textures::NUMBER_OF_TEXTURES);
constexpr std::array<std::string_view, TEXTURE_NUMBER> texture_names = {{
    "air",
    "stone",
    "dirt",
    "grass_side",
    "grass_top",
    "glass",
    "iron_block",
    "redstone_lamp",
    "diamond_ore",
    "slime_block",
    "oak_log_side",
    "oak_log_top",
    "oak_leaves",
}};

constexpr uint8_t TEXTURE_SIZE = 16;
constexpr uint64_t ATLAS_SIZE = ATLAS_DIMS * TEXTURE_SIZE;

constexpr glm::u32vec2 TEX(const Textures t) {
    uint64_t tex_index = static_cast<uint64_t>(t) - 1;
    return {tex_index % ATLAS_DIMS, tex_index / ATLAS_DIMS};
}
