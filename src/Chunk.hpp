#pragma once

// GLM EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

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
        glm::ivec3(0, 0, -CHUNK_SIZE), // Front (-Z)
        glm::ivec3(-CHUNK_SIZE, 0, 0), // Left  (-X)
        glm::ivec3(0, -CHUNK_SIZE, 0), // Bottom(-Y)
        glm::ivec3(0, 0, CHUNK_SIZE),  // Back  (+Z)
        glm::ivec3(CHUNK_SIZE, 0, 0),  // Right (+X)
        glm::ivec3(0, CHUNK_SIZE, 0),  // Top   (+Y)
    };

    static constexpr glm::ivec3 blockPosToChunkPos(const glm::ivec3 &_block_pos) {
        return glm::ivec3(
            _block_pos.x + _block_pos.x % CHUNK_SIZE,
            _block_pos.y + _block_pos.y % CHUNK_SIZE,
            _block_pos.z + _block_pos.z % CHUNK_SIZE);
    }

    static constexpr uint chunkDistance(const glm::ivec3 &_a, const glm::ivec3 &_b) {
        return std::sqrt(std::pow(_a.x - _b.x, 2) + std::pow(_a.y - _b.y, 2) + std::pow(_a.z - _b.z, 2));
    }

    enum class GenType {
        SUPERFLAT,
    };

private:
    glm::ivec3 m_pos;
    std::array<Block, CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE> m_blocks;

    AABB<float> m_aabb;
    Mesh m_mesh;

    static inline size_t posToBlockI(uint x, uint y, uint z) { return (y * CHUNK_SIZE + z) * CHUNK_SIZE + x; }
    static inline size_t posToBlockI(const glm::uvec3 &_relative_pos) { return (_relative_pos.y * CHUNK_SIZE + _relative_pos.z) * CHUNK_SIZE + _relative_pos.x; }

    inline void foreachBlock(std::function<void(const glm::uvec3 &pos, const glm::ivec3 &world_pos, Block &block)> _func) {
        glm::uvec3 pos(0);
        glm::ivec3 world_pos;
        world_pos.y = m_pos.y;
        for (pos.y = 0; pos.y < CHUNK_SIZE; pos.y++) {
            world_pos.z = m_pos.z;
            for (pos.z = 0; pos.z < CHUNK_SIZE; pos.z++) {
                world_pos.x = m_pos.x;
                for (pos.x = 0; pos.x < CHUNK_SIZE; pos.x++) {
                    _func(pos, world_pos, m_blocks[posToBlockI(pos)]);
                    world_pos.x++;
                }
                world_pos.z++;
            }
            world_pos.y++;
        }
    }

public:
    std::array<const Chunk *, 6> m_neighbours{nullptr};

    Chunk(const glm::ivec3 &_chunk_pos, GenType _type);
    ~Chunk();

    inline const glm::ivec3 &getPos() const { return m_pos; }
    inline glm::ivec3 &getPos() { return m_pos; }
    inline const Block &getBlock(const glm::ivec3 &_block_pos) const { return m_blocks[posToBlockI(_block_pos - m_pos)]; }
    inline Block &getBlock(const glm::ivec3 &_block_pos) { return m_blocks[posToBlockI(_block_pos - m_pos)]; }

    // bool isVisible(const Camera &_camera); // Check if the chunk is in the frustum

    void recomputeBlockNeighbours();
    void buildMesh();
    inline void render(ShaderProgram &_shader) {
        _shader.set("texture_i", -1);
        _shader.set("texture_sampler", -1);

        _shader.set("model", glm::mat4(1.));
        _shader.set("has_normals", 1);
        m_mesh.render();
    }
    inline void renderDebugBox(ShaderProgram &_shader) {
        // m_aabb = AABB<float>(glm::vec3(m_pos), glm::vec3(m_pos) + glm::vec3(CHUNK_SIZE));
        // m_aabb.initShaderData();
        _shader.set("texture_i", -1);
        _shader.set("texture_sampler", -1);

        _shader.set("model", glm::mat4(1.));
        _shader.set("has_normals", 0);
        m_aabb.render();
    }
    inline void clear() {
        m_mesh.clear();
        m_aabb.clearShaderData();
    }
};
