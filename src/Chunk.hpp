#pragma once

// USUAL INCLUDES
#include "Block.hpp"
#include "Camera.hpp"
#include "Mesh.hpp"
#include "ShaderProgram.hpp"
#include <array>
#include <cstdint>
#include <functional>
#include <iostream>

class Chunk {
public:
    static constexpr uint8_t CHUNK_SIZE = 32; // A chunk is 32x32x32 blocks
    static constexpr std::array<glm::ivec3, 6> NEIGHBOURS_POS{
        glm::ivec3(0, 0, -CHUNK_SIZE),
        glm::ivec3(0, 0, CHUNK_SIZE),
        glm::ivec3(-CHUNK_SIZE, 0, 0),
        glm::ivec3(CHUNK_SIZE, 0, 0),
        glm::ivec3(0, -CHUNK_SIZE, 0),
        glm::ivec3(0, CHUNK_SIZE, 0),
    };

    static constexpr glm::ivec3 blockPosToChunkPos(const glm::ivec3 &_block_pos) {
        return glm::ivec3(
            _block_pos.x + _block_pos.x % CHUNK_SIZE,
            _block_pos.y + _block_pos.y % CHUNK_SIZE,
            _block_pos.z + _block_pos.z % CHUNK_SIZE);
    }

    enum class GenType {
        SUPERFLAT,
    };

private:
    glm::ivec3 m_pos;
    std::array<Block::BlockType, CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE> m_blocks;
    Mesh m_mesh;

    static inline size_t posToBlockI(uint x, uint y, uint z) { return (y * CHUNK_SIZE + z) * CHUNK_SIZE + x; }
    static inline size_t posToBlockI(const glm::uvec3 &_relative_pos) { return (_relative_pos.y * CHUNK_SIZE + _relative_pos.z) * CHUNK_SIZE + _relative_pos.x; }

    inline void foreachBlockRelative(std::function<void(uint8_t x, uint8_t y, uint8_t z, Block::BlockType &type)> _func) {
        for (uint8_t y = 0; y < CHUNK_SIZE; y++)
            for (uint8_t z = 0; z < CHUNK_SIZE; z++)
                for (uint8_t x = 0; x < CHUNK_SIZE; x++)
                    _func(x, y, z, m_blocks[posToBlockI(x, y, z)]);
    }
    inline void foreachBlockWorld(std::function<void(int world_x, int world_y, int world_z, Block::BlockType &type)> _func) {
        for (uint8_t y = 0; y < CHUNK_SIZE; y++) {
            int world_y = m_pos.y + y;
            for (uint8_t z = 0; z < CHUNK_SIZE; z++) {
                int world_z = m_pos.z + z;
                for (uint8_t x = 0; x < CHUNK_SIZE; x++) {
                    int world_x = m_pos.x + x;
                    _func(world_x, world_y, world_z, m_blocks[posToBlockI(x, y, z)]);
                }
            }
        }
    }

public:
    Chunk(const glm::ivec3 &_chunk_pos, GenType _type);

    inline const glm::ivec3 &getPos() const { return m_pos; }
    inline glm::ivec3 &getPos() { return m_pos; }
    inline const Block::BlockType &getBlockType(const glm::ivec3 &_block_pos) const { return m_blocks[posToBlockI(_block_pos - m_pos)]; }
    inline Block::BlockType &getBlockType(const glm::ivec3 &_block_pos) { return m_blocks[posToBlockI(_block_pos - m_pos)]; }

    // bool isVisible(const Camera &_camera); // Check if the chunk is in the frustum

    void buildMesh();
    inline void render(ShaderProgram &_shader) {
        _shader.set("texture_i", -1);
        _shader.set("texture_sampler", -1);

        _shader.set("model", glm::mat4(1.));
        _shader.set("has_normals", 1);
        m_mesh.render();
    }
    inline void clear() { m_mesh.clear(); }
};
