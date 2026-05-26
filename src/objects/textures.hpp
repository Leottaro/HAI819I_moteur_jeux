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

constexpr std::array<std::string_view, TEXTURE_NUMBER> texture_names = {{"stone",
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
                                                                         "honey_block_top",
                                                                         "honey_block_side",
                                                                         "honey_block_bottom",
                                                                         "ice",
                                                                         "water",
                                                                         "pierre_de_lit"}};