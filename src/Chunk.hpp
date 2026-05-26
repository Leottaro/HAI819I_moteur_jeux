#pragma once

// USUAL INCLUDES
#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

#include "AABB.hpp"
#include "Block.hpp"
#include "GreyMap.hpp"
#include "Helpers.hpp"

class World;
class ChunkRenderer;

enum class GenType : uint8_t {
    DEBUG_,
    SUPERFLAT,
    SURMONDE,
    COUNT
};

static constexpr const char* GenTypeNames[] = {
    "Debug",
    "Superflat",
    "Surmonde"};

class Chunk {
   public:
    static constexpr uint8_t CHUNK_SIZE = 32;  // The size of a cubic chunk
    static constexpr size_t NB_BLOCKS = CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE;

    static constexpr std::array<glm::ivec3, 6> NEIGHBOURS_POS{
        glm::ivec3(0, 0, -CHUNK_SIZE),  // Front (-Z)
        glm::ivec3(-CHUNK_SIZE, 0, 0),  // Left  (-X)
        glm::ivec3(0, -CHUNK_SIZE, 0),  // Bottom(-Y)
        glm::ivec3(0, 0, CHUNK_SIZE),   // Back  (+Z)
        glm::ivec3(CHUNK_SIZE, 0, 0),   // Right (+X)
        glm::ivec3(0, CHUNK_SIZE, 0),   // Top   (+Y)
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

    static constexpr uint chunkSquaredDistance(const glm::ivec3& _chunk1, const glm::ivec3& _chunk2) {
        uint dist = (_chunk1.x - _chunk2.x) * (_chunk1.x - _chunk2.x) +
                    (_chunk1.y - _chunk2.y) * (_chunk1.y - _chunk2.y) +
                    (_chunk1.z - _chunk2.z) * (_chunk1.z - _chunk2.z);
        return dist / (CHUNK_SIZE * CHUNK_SIZE);
    }

   private:
    World* m_world{nullptr};
    glm::ivec3 m_pos;
    std::array<Block, NB_BLOCKS> m_blocks{initBlocks()};
    AABB<float> m_aabb;
    // std::optional<GreyMap> m_heightmap;
    bool* m_should_rebuild_mesh{nullptr};  // For the ChunkRenderer

    inline static constexpr size_t posToBlockI(uint8_t x, uint8_t y, uint8_t z) { return (y * CHUNK_SIZE + x) * CHUNK_SIZE + z; }
    inline static constexpr size_t posToBlockI(const glm::u8vec3& _relative_pos) { return (_relative_pos.x * CHUNK_SIZE + _relative_pos.z) * CHUNK_SIZE + _relative_pos.y; }
    static constexpr std::array<int, 6> BLOCK_NEIGHBOUR_I_OFFSET{
        -CHUNK_SIZE,              // Front (-Z)
        -CHUNK_SIZE* CHUNK_SIZE,  // Left  (-X)
        -1,                       // Bottom(-Y)
        CHUNK_SIZE,               // Back  (+Z)
        CHUNK_SIZE* CHUNK_SIZE,   // Right (+X)
        1,                        // Top   (+Y)
    };
    static constexpr std::array<Block, NB_BLOCKS> initBlocks() {
        std::array<Block, NB_BLOCKS> blocks;
        std::array<bool, 3> neighbour_exists{false};
        glm::u8vec3 local_pos;
        int block_i = -1;
        for (local_pos.x = 0; local_pos.x < CHUNK_SIZE; local_pos.x++) {
            neighbour_exists[1] = local_pos.x > 0;
            for (local_pos.z = 0; local_pos.z < CHUNK_SIZE; local_pos.z++) {
                neighbour_exists[0] = local_pos.z > 0;
                for (local_pos.y = 0; local_pos.y < CHUNK_SIZE; local_pos.y++) {
                    neighbour_exists[2] = local_pos.y > 0;
                    block_i++;
                    for (uint8_t _face_i = 0; _face_i < 3; _face_i++) {
                        if (neighbour_exists[_face_i]) {
                            int neighbour_i = block_i + BLOCK_NEIGHBOUR_I_OFFSET[_face_i];
                            blocks[block_i].m_neighbours[_face_i] = &blocks[neighbour_i];
                            blocks[neighbour_i].m_neighbours[OPPOSITE_FACE[_face_i]] = &blocks[block_i];
                        }
                    }
                }
            }
        }
        return blocks;
    }

    inline Block& getBlock(const glm::ivec3& _block_pos) { return m_blocks[posToBlockI(_block_pos - m_pos)]; }
    void generate(GenType _type);

   public:
    std::array<Chunk*, 6> m_neighbours{nullptr};

    Chunk(Chunk&& other) : m_world(other.m_world), m_pos(other.m_pos), m_blocks(other.m_blocks), m_aabb(other.m_aabb), m_neighbours(other.m_neighbours) {
        for (uint i = 0; i < 6; i++)
            if (m_neighbours[i] != nullptr)
                m_neighbours[i]->m_neighbours[OPPOSITE_FACE[i]] = this;
        other.m_world = nullptr;
        other.m_neighbours.fill(nullptr);
    }
    Chunk& operator=(Chunk&& other) {
        if (this == &other)
            return *this;

        m_world = other.m_world;
        m_pos = other.m_pos;
        m_blocks = other.m_blocks;
        m_aabb = other.m_aabb;
        // m_heightmap = other.m_heightmap;
        m_neighbours = other.m_neighbours;

        for (uint i = 0; i < 6; i++)
            if (m_neighbours[i] != nullptr)
                m_neighbours[i]->m_neighbours[OPPOSITE_FACE[i]] = this;

        other.m_world = nullptr;
        other.m_neighbours.fill(nullptr);
        return *this;
    }
    Chunk(const Chunk&) = delete;
    Chunk& operator=(const Chunk&) = delete;
    ~Chunk() = default;

    Chunk() {}
    Chunk(World* _world, const glm::ivec3& _chunk_pos, GenType _type);

    inline const glm::ivec3& getPos() const { return m_pos; }
    inline const AABB<float>& getAABB() const { return m_aabb; }
    inline Chunk* getNeighbour(uint8_t _face_i) const { return m_neighbours[_face_i]; }

    inline const Block& getBlock(const glm::ivec3& _block_pos) const { return m_blocks[posToBlockI(_block_pos - m_pos)]; }
    Chunk* getChunk(const glm::vec3& _pos) const;
    Block* findBlock(const glm::ivec3& _block_pos) const;
    std::vector<const Block*> findBlockLine(const glm::vec3& _start, const glm::vec3& _end) const;

    inline void setBlockType(const glm::ivec3& _block_pos, BlockType _type) {
        Block& block = getBlock(_block_pos);
        BlockType& block_type = block.getType();
        if (block_type == BlockType::PierreDeLit) {
            std::cout << "On ne casse pas de pierre de lit..." << std::endl;
            return;
        }
        if (block_type == _type)
            return;

        if (m_should_rebuild_mesh != nullptr) {
            *m_should_rebuild_mesh = true;
        }

        if (getBlockTypeData(block_type).transparence == getBlockTypeData(_type).transparence)  // On skip si ils ont la même transparence parce que pas besoin d'actualiser la mesh des potentiels chunks d'à côté
            return;
        for (uint axis = 0; axis < 3; axis++) {
            uint face_axis = (axis + 1) % 3;  // pour match l'ordre des faces
            if (_block_pos[axis] == m_pos[axis] && m_neighbours[face_axis] != nullptr && m_neighbours[face_axis]->m_should_rebuild_mesh != nullptr)
                *m_neighbours[face_axis]->m_should_rebuild_mesh = true;
            if (_block_pos[axis] == m_pos[axis] + CHUNK_SIZE - 1 && m_neighbours[face_axis + 3] != nullptr && m_neighbours[face_axis + 3]->m_should_rebuild_mesh != nullptr)
                *m_neighbours[face_axis + 3]->m_should_rebuild_mesh = true;
        }
        block_type = _type;
    }

    void updateBlockNeighbours(uint8_t _face_i);

    friend World;
    friend ChunkRenderer;
};

class ChunkRenderer {
    struct ChunkVertex {
        MathHelpers::u8pvec3 position;
        MathHelpers::i8pvec3 normal;
        MathHelpers::i8pvec3 tangent;
        MathHelpers::i8pvec3 bitangent;
        BlockTexture block_tex;
        MathHelpers::fpvec2 uv;
    };

    static constexpr size_t MAX_VERTICES = Chunk::NB_BLOCKS * 6 * 4;
    static constexpr size_t MAX_TRIANGLES = Chunk::NB_BLOCKS * 6 * 2;
    static constexpr size_t VERTICES_PREALLOCATION = MAX_VERTICES / Chunk::CHUNK_SIZE;
    static constexpr size_t TRIANGLES_PREALLOCATION = MAX_VERTICES / Chunk::CHUNK_SIZE;

    GLuint m_opaque_VAO{0};
    GLuint m_opaque_VBO{0};
    GLuint m_opaque_EBO{0};
    size_t m_opaque_vertices{0};
    size_t m_opaque_triangles{0};

    GLuint m_translucent_VAO{0};
    GLuint m_translucent_VBO{0};
    GLuint m_translucent_EBO{0};
    size_t m_translucent_vertices{0};
    size_t m_translucent_triangles{0};

    Chunk* m_chunk{nullptr};
    glm::ivec3 m_pos{Chunk::CHUNK_SIZE - 1};
    AABB<float> m_aabb{};
    bool m_should_rebuild_mesh{true};

    inline void setChunk(Chunk* _chunk) {
        if (_chunk == m_chunk)
            return;

        if (m_chunk != nullptr) {
            m_chunk->m_should_rebuild_mesh = nullptr;
        }

        m_chunk = _chunk;
        if (m_chunk != nullptr) {
            m_pos = m_chunk->m_pos;
            m_aabb = m_chunk->m_aabb;
            m_chunk->m_should_rebuild_mesh = &m_should_rebuild_mesh;
        }
    }

   public:
    ChunkRenderer(ChunkRenderer&& other) : m_opaque_VAO(other.m_opaque_VAO), m_opaque_VBO(other.m_opaque_VBO), m_opaque_EBO(other.m_opaque_EBO), m_opaque_vertices(other.m_opaque_vertices), m_opaque_triangles(other.m_opaque_triangles), m_translucent_VAO(other.m_translucent_VAO), m_translucent_VBO(other.m_translucent_VBO), m_translucent_EBO(other.m_translucent_EBO), m_translucent_vertices(other.m_translucent_vertices), m_translucent_triangles(other.m_translucent_triangles) {
        other.setChunk(nullptr);
        other.m_opaque_VAO = other.m_opaque_VBO = other.m_opaque_EBO = other.m_opaque_vertices = other.m_opaque_triangles = other.m_translucent_VAO = other.m_translucent_VBO = other.m_translucent_EBO = other.m_translucent_vertices = 0;
    }
    ChunkRenderer& operator=(ChunkRenderer&& other) {
        if (this == &other)
            return *this;
        clearShaderData();

        m_opaque_VAO = other.m_opaque_VAO;
        m_opaque_VBO = other.m_opaque_VBO;
        m_opaque_EBO = other.m_opaque_EBO;
        m_opaque_vertices = other.m_opaque_vertices;
        m_opaque_triangles = other.m_opaque_triangles;
        m_translucent_VAO = other.m_translucent_VAO;
        m_translucent_VBO = other.m_translucent_VBO;
        m_translucent_EBO = other.m_translucent_EBO;
        m_translucent_vertices = other.m_translucent_vertices;
        m_translucent_triangles = other.m_translucent_triangles;

        other.setChunk(nullptr);
        other.m_opaque_VAO = other.m_opaque_VBO = other.m_opaque_EBO = other.m_opaque_vertices = other.m_opaque_triangles = other.m_translucent_VAO = other.m_translucent_VBO = other.m_translucent_EBO = other.m_translucent_vertices = 0;
        return *this;
    }
    ChunkRenderer(const ChunkRenderer& other) = delete;
    ChunkRenderer& operator=(const ChunkRenderer& other) = delete;
    ~ChunkRenderer() {
        setChunk(nullptr);
        clearShaderData();
    }

    ChunkRenderer() {}
    ChunkRenderer(Chunk* _chunk, const glm::vec3& _cam_pos) {
        setChunk(_chunk);
        initShaderData();
        updateShaderData(_cam_pos);
    }

    inline bool& shouldRebuildMesh() { return m_should_rebuild_mesh; }
    inline glm::ivec3& getPos() { return m_pos; }
    inline AABB<float>& getAABB() { return m_aabb; }

    inline const bool& shouldRebuildMesh() const { return m_should_rebuild_mesh; }
    inline const glm::ivec3& getPos() const { return m_pos; }
    inline const AABB<float>& getAABB() const { return m_aabb; }
    inline size_t getOpaqueTriangles() const { return m_opaque_triangles; }
    inline size_t getTranslucentTriangles() const { return m_translucent_triangles; }

    void initShaderData();
    void updateShaderData(const glm::vec3& _cam_pos);
    void renderOpaque() const;
    void renderTranslucent() const;
    void clearShaderData();

    friend Chunk;
};