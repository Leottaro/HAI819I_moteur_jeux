#pragma once

#include <array>
#include <cstddef>

enum class Textures : size_t {
    air = 0,
    stone,
    dirt,
    grass_side,
    grass_top,
    glass,
    iron_block,
    redstone_lamp,
    diamond_ore,
    NUMBER_OF_TEXTURES
};

constexpr size_t TEXTURE_NUMBER = static_cast<size_t>(Textures::NUMBER_OF_TEXTURES);
constexpr std::array<char*, TEXTURE_NUMBER> texture_names = {{
    "air",
    "stone",
    "dirt",
    "grass_side",
    "grass_top",
    "glass",
    "iron_block",
    "redstone_lamp",
    "diamond_ore",
}};

constexpr size_t texture_size = 16;
constexpr size_t atlas_dims = 4;
constexpr size_t img_size = atlas_dims * texture_size;
