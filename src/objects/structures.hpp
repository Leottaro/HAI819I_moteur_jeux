#pragma once

#include <array>
#include <glm/glm.hpp>

#include "src/objects/blocks.hpp"

struct BlockPos {
    BlockType block;
    glm::i8vec3 pos;
};

constexpr std::array<BlockPos, 67> TREE_DATA = {{
    {BlockType::Dirt, {0, 0, 0}},
    {BlockType::OakLog, {0, 1, 0}},
    {BlockType::OakLog, {0, 2, 0}},
    {BlockType::OakLog, {0, 3, 0}},
    {BlockType::OakLog, {0, 4, 0}},
    {BlockType::OakLog, {0, 5, 0}},
    {BlockType::OakLeaves, {0, 6, 0}},
    {BlockType::OakLeaves, {1, 6, 0}},
    {BlockType::OakLeaves, {0, 6, 1}},
    {BlockType::OakLeaves, {-1, 6, 0}},
    {BlockType::OakLeaves, {0, 6, -1}},
    {BlockType::OakLeaves, {1, 5, 0}},
    {BlockType::OakLeaves, {0, 5, 1}},
    {BlockType::OakLeaves, {-1, 5, 0}},
    {BlockType::OakLeaves, {0, 5, -1}},
    {BlockType::OakLeaves, {1, 5, 1}},
    {BlockType::OakLeaves, {-1, 5, 1}},
    {BlockType::OakLeaves, {1, 5, -1}},
    {BlockType::OakLeaves, {-1, 5, -1}},

    {BlockType::OakLeaves, {1, 4, 0}},
    {BlockType::OakLeaves, {0, 4, 1}},
    {BlockType::OakLeaves, {-1, 4, 0}},
    {BlockType::OakLeaves, {0, 4, -1}},
    {BlockType::OakLeaves, {1, 4, 1}},
    {BlockType::OakLeaves, {-1, 4, 1}},
    {BlockType::OakLeaves, {1, 4, -1}},
    {BlockType::OakLeaves, {-1, 4, -1}},
    {BlockType::OakLeaves, {2, 4, 2}},
    {BlockType::OakLeaves, {2, 4, 1}},
    {BlockType::OakLeaves, {2, 4, 0}},
    {BlockType::OakLeaves, {2, 4, -1}},
    {BlockType::OakLeaves, {2, 4, -2}},
    {BlockType::OakLeaves, {1, 4, -2}},
    {BlockType::OakLeaves, {0, 4, -2}},
    {BlockType::OakLeaves, {-1, 4, -2}},
    {BlockType::OakLeaves, {-2, 4, -2}},
    {BlockType::OakLeaves, {-2, 4, -1}},
    {BlockType::OakLeaves, {-2, 4, 0}},
    {BlockType::OakLeaves, {-2, 4, 1}},
    {BlockType::OakLeaves, {-2, 4, 2}},
    {BlockType::OakLeaves, {-1, 4, 2}},
    {BlockType::OakLeaves, {0, 4, 2}},
    {BlockType::OakLeaves, {1, 4, 2}},

    {BlockType::OakLeaves, {1, 3, 0}},
    {BlockType::OakLeaves, {0, 3, 1}},
    {BlockType::OakLeaves, {-1, 3, 0}},
    {BlockType::OakLeaves, {0, 3, -1}},
    {BlockType::OakLeaves, {1, 3, 1}},
    {BlockType::OakLeaves, {-1, 3, 1}},
    {BlockType::OakLeaves, {1, 3, -1}},
    {BlockType::OakLeaves, {-1, 3, -1}},
    {BlockType::OakLeaves, {2, 3, 2}},
    {BlockType::OakLeaves, {2, 3, 1}},
    {BlockType::OakLeaves, {2, 3, 0}},
    {BlockType::OakLeaves, {2, 3, -1}},
    {BlockType::OakLeaves, {2, 3, -2}},
    {BlockType::OakLeaves, {1, 3, -2}},
    {BlockType::OakLeaves, {0, 3, -2}},
    {BlockType::OakLeaves, {-1, 3, -2}},
    {BlockType::OakLeaves, {-2, 3, -2}},
    {BlockType::OakLeaves, {-2, 3, -1}},
    {BlockType::OakLeaves, {-2, 3, 0}},
    {BlockType::OakLeaves, {-2, 3, 1}},
    {BlockType::OakLeaves, {-2, 3, 2}},
    {BlockType::OakLeaves, {-1, 3, 2}},
    {BlockType::OakLeaves, {0, 3, 2}},
    {BlockType::OakLeaves, {1, 3, 2}},

}};

// TODO : c'est nul de faire avec de la chance
constexpr float TREE_CHANCE = 0.001f * RAND_MAX;
