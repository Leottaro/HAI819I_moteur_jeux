#pragma once

// USUAL INCLUDES
#include "Block.hpp"
#include "ShaderProgram.hpp"
#include "AABB.hpp"
#include <array>
#include <cstdint>

class Chunk {
public:
    static constexpr uint8_t CHUNK_SIZE = 32; // A chunk is 32x32x32 blocks
    static constexpr uint MAX_VERTICES = CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE * 6 * 4;
    static constexpr uint MAX_TRIANGLES = CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE * 6 * 2;

    static constexpr std::array<glm::ivec3, 6> NEIGHBOURS_POS{
        glm::ivec3(0, 0, -CHUNK_SIZE), // Front (-Z)
        glm::ivec3(-CHUNK_SIZE, 0, 0), // Left  (-X)
        glm::ivec3(0, -CHUNK_SIZE, 0), // Bottom(-Y)
        glm::ivec3(0, 0, CHUNK_SIZE),  // Back  (+Z)
        glm::ivec3(CHUNK_SIZE, 0, 0),  // Right (+X)
        glm::ivec3(0, CHUNK_SIZE, 0),  // Top   (+Y)
    };

    static constexpr glm::ivec3 posToChunkPos(const glm::vec3 &_pos) {
        return blockPosToChunkPos(glm::ivec3(_pos.x, _pos.y, _pos.z));
    }
    static constexpr glm::ivec3 blockPosToChunkPos(const glm::ivec3 &_block_pos) {
        return glm::ivec3(
            (_block_pos.x < 0 && _block_pos.x % CHUNK_SIZE != 0 ? _block_pos.x / CHUNK_SIZE - 1 : _block_pos.x / CHUNK_SIZE) * CHUNK_SIZE,
            (_block_pos.y < 0 && _block_pos.y % CHUNK_SIZE != 0 ? _block_pos.y / CHUNK_SIZE - 1 : _block_pos.y / CHUNK_SIZE) * CHUNK_SIZE,
            (_block_pos.z < 0 && _block_pos.z % CHUNK_SIZE != 0 ? _block_pos.z / CHUNK_SIZE - 1 : _block_pos.z / CHUNK_SIZE) * CHUNK_SIZE);
    }

    static constexpr float chunkDistance(const glm::vec3 &_a, const glm::vec3 &_b) {
        float dist = std::sqrt(std::pow(_a.x - _b.x - 16, 2) + std::pow(_a.y - _b.y - 16, 2) + std::pow(_a.z - _b.z - 16, 2));
        return dist / CHUNK_SIZE;
    }

    enum class GenType {
        SUPERFLAT,
    };

private:
    GLuint m_VAO = 0;
    GLuint m_VBO = 0;
    GLuint m_EBO = 0;
    size_t m_vertices_count = 0, m_triangles_count = 0;

    glm::ivec3 m_pos;
    std::array<Block, CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE> m_blocks;
    AABB<float> m_aabb;

    static inline size_t posToBlockI(uint x, uint y, uint z) { return (y * CHUNK_SIZE + z) * CHUNK_SIZE + x; }
    static inline size_t posToBlockI(const glm::uvec3 &_relative_pos) { return (_relative_pos.y * CHUNK_SIZE + _relative_pos.z) * CHUNK_SIZE + _relative_pos.x; }
    static constexpr std::array<int, 6> BLOCK_NEIGHBOUR_I_OFFSET{
        -CHUNK_SIZE,             // Front (-Z)
        -1,                      // Left  (-X)
        -CHUNK_SIZE *CHUNK_SIZE, // Bottom(-Y)
        CHUNK_SIZE,              // Back  (+Z)
        1,                       // Right (+X)
        CHUNK_SIZE *CHUNK_SIZE,  // Top   (+Y)
    };

    void initNeighbours();
    void generate(GenType _type);

public:
    std::array<Chunk *, 6> m_neighbours{nullptr};

    Chunk(Chunk &&) = delete;
    Chunk(const Chunk &) = delete;
    Chunk &operator=(const Chunk &) = delete;
    Chunk &operator=(Chunk &&) = delete;
    Chunk(const glm::ivec3 &_chunk_pos, GenType _type);
    ~Chunk() { clearShaderData(); }

    inline const glm::ivec3 &getPos() const { return m_pos; }
    inline const AABB<float> &getAABB() { return m_aabb; }
    inline const Block &getBlock(const glm::ivec3 &_block_pos) const { return m_blocks[posToBlockI(_block_pos - m_pos)]; }
    inline Block &getBlock(const glm::ivec3 &_block_pos) { return m_blocks[posToBlockI(_block_pos - m_pos)]; }

    // bool isVisible(const Camera &_camera); // Check if the chunk is in the frustum
    void updateBlockNeighbours(uint8_t _face_i);

    void initShaderData();
    void updateShaderData();
    void render();
    void renderDebugBox();
    void clearShaderData();
};