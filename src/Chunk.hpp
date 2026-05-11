#pragma once

// USUAL INCLUDES
#include <array>
#include <cstdint>
#include <optional>
#include <vector>

#include "AABB.hpp"
#include "Block.hpp"
#include "GreyMap.hpp"
#include "ShaderProgram.hpp"
#include "Texture.hpp"

class World;

enum class GenType {
    DEBUG_,
    SUPERFLAT,
    OVERWORLD,
    COUNT
};

static constexpr const char* GenTypeNames[] = {
    "Debug",
    "Superflat",
    "Overworld"};

class Chunk {
public:
    static constexpr uint8_t CHUNK_SIZE = 32; // The size of a cubic chunk
    static constexpr size_t NB_BLOCKS = CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE;
    static constexpr size_t MAX_VERTICES = NB_BLOCKS * 6 * 4;
    static constexpr size_t MAX_TRIANGLES = NB_BLOCKS * 6 * 2;

    static constexpr std::array<glm::ivec3, 6> NEIGHBOURS_POS{
        glm::ivec3(0, 0, -CHUNK_SIZE), // Front (-Z)
        glm::ivec3(-CHUNK_SIZE, 0, 0), // Left  (-X)
        glm::ivec3(0, -CHUNK_SIZE, 0), // Bottom(-Y)
        glm::ivec3(0, 0, CHUNK_SIZE),  // Back  (+Z)
        glm::ivec3(CHUNK_SIZE, 0, 0),  // Right (+X)
        glm::ivec3(0, CHUNK_SIZE, 0),  // Top   (+Y)
    };

    static constexpr glm::ivec3 posToChunkPos(const glm::vec3& _pos) {
        return blockPosToChunkPos(Block::posToBlockPos(_pos));
    }
    static constexpr glm::ivec3 blockPosToChunkPos(const glm::ivec3& _block_pos) {
        return glm::ivec3(
            (_block_pos.x < 0 && _block_pos.x % CHUNK_SIZE != 0 ? _block_pos.x / CHUNK_SIZE - 1 : _block_pos.x / CHUNK_SIZE) * CHUNK_SIZE,
            (_block_pos.y < 0 && _block_pos.y % CHUNK_SIZE != 0 ? _block_pos.y / CHUNK_SIZE - 1 : _block_pos.y / CHUNK_SIZE) * CHUNK_SIZE,
            (_block_pos.z < 0 && _block_pos.z % CHUNK_SIZE != 0 ? _block_pos.z / CHUNK_SIZE - 1 : _block_pos.z / CHUNK_SIZE) * CHUNK_SIZE);
    }
    static constexpr float chunkDistance(const glm::vec3& _a, const glm::vec3& _b) {
        float dist = std::sqrt(std::pow(_a.x - _b.x - 16, 2) + std::pow(_a.y - _b.y - 16, 2) + std::pow(_a.z - _b.z - 16, 2));
        return dist / CHUNK_SIZE;
    }

private:
    GLuint m_opaque_VAO = 0;
    GLuint m_opaque_VBO = 0;
    GLuint m_opaque_EBO = 0;
    size_t m_opaque_vertices = 0;
    size_t m_opaque_triangles = 0;

    GLuint m_translucent_VAO = 0;
    GLuint m_translucent_VBO = 0;
    GLuint m_translucent_EBO = 0;
    size_t m_translucent_vertices = 0;
    size_t m_translucent_triangles = 0;

    World* m_world;
    glm::ivec3 m_pos;
    std::array<Block, NB_BLOCKS> m_blocks;
    AABB<float> m_aabb;

    std::optional<GreyMap> m_heightmap;

    static inline size_t posToBlockI(uint x, uint y, uint z) { return (y * CHUNK_SIZE + z) * CHUNK_SIZE + x; }
    static inline size_t posToBlockI(const glm::uvec3& _relative_pos) { return (_relative_pos.y * CHUNK_SIZE + _relative_pos.z) * CHUNK_SIZE + _relative_pos.x; }
    static constexpr std::array<int, 6> BLOCK_NEIGHBOUR_I_OFFSET{
        -CHUNK_SIZE,              // Front (-Z)
        -1,                       // Left  (-X)
        -CHUNK_SIZE * CHUNK_SIZE, // Bottom(-Y)
        CHUNK_SIZE,               // Back  (+Z)
        1,                        // Right (+X)
        CHUNK_SIZE * CHUNK_SIZE,  // Top   (+Y)
    };

    inline Block& getBlock(const glm::ivec3& _block_pos) { return m_blocks[posToBlockI(_block_pos - m_pos)]; }
    void initNeighbours();
    void generate(GenType _type);

public:
    std::array<Chunk*, 6> m_neighbours{nullptr};
    size_t nb_translucent_block{0};
    bool should_rebuild_mesh{true};

    Chunk(Chunk&&) = delete;
    Chunk(const Chunk&) = delete;
    Chunk& operator=(const Chunk&) = delete;
    Chunk& operator=(Chunk&&) = delete;
    Chunk(World* _world, const glm::ivec3& _chunk_pos, GenType _type);
    ~Chunk() { clearShaderData(); }

    inline const glm::ivec3& getPos() const { return m_pos; }
    inline const AABB<float>& getAABB() const { return m_aabb; }
    inline Chunk* getNeighbour(uint8_t _face_i) const { return m_neighbours[_face_i]; }

    inline const Block& getBlock(const glm::ivec3& _block_pos) const { return m_blocks[posToBlockI(_block_pos - m_pos)]; }
    Chunk* getChunk(const glm::vec3& _pos) const;
    Block* findBlock(const glm::ivec3& _block_pos) const;
    void findSolidBlocks(const glm::vec3& _start, const glm::vec3& _end, std::vector<const Block*>& blocks) const;

    inline void setBlockType(const glm::ivec3& _block_pos, BlockType _type) {
        Block& block = getBlock(_block_pos);
        BlockType& block_type = block.getType();
        int translucent_diff = (getBlockTypeData(_type).transparence == BlockTransparence::TRANSLUCENT) - (getBlockTypeData(block_type).transparence == BlockTransparence::TRANSLUCENT);
        if (translucent_diff != 0) {
            nb_translucent_block += translucent_diff;
            should_rebuild_mesh = true;
        }
        block_type = _type;
    }

    // bool isVisible(const Camera &_camera); // Check if the chunk is in the frustum
    void updateBlockNeighbours(uint8_t _face_i);

    void initShaderData();
    void updateShaderData(const glm::vec3& _cam_pos);
    void renderOpaque();
    void renderTranslucent();
    void clearShaderData();

    friend World;
};