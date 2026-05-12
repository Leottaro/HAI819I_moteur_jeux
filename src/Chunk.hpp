#pragma once

// USUAL INCLUDES
#include <array>
#include <cstdint>
#include <optional>
#include <vector>
#include <memory>

#include "AABB.hpp"
#include "Block.hpp"
#include "GreyMap.hpp"
#include "Helpers.hpp"

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
    std::array<Block, NB_BLOCKS> m_blocks{initBlocks()};
    AABB<float> m_aabb;
    std::optional<GreyMap> m_heightmap;

    static constexpr size_t posToBlockI(uint x, uint y, uint z) { return (y * CHUNK_SIZE + z) * CHUNK_SIZE + x; }
    static constexpr size_t posToBlockI(const glm::uvec3& _relative_pos) { return (_relative_pos.y * CHUNK_SIZE + _relative_pos.z) * CHUNK_SIZE + _relative_pos.x; }
    static constexpr std::array<int, 6> BLOCK_NEIGHBOUR_I_OFFSET{
        -CHUNK_SIZE,              // Front (-Z)
        -1,                       // Left  (-X)
        -CHUNK_SIZE * CHUNK_SIZE, // Bottom(-Y)
        CHUNK_SIZE,               // Back  (+Z)
        1,                        // Right (+X)
        CHUNK_SIZE * CHUNK_SIZE,  // Top   (+Y)
    };
    static constexpr std::array<Block, NB_BLOCKS> initBlocks() {
        std::array<Block, NB_BLOCKS> blocks;
        std::array<bool, 3> neighbour_exists{false};
        glm::u8vec3 local_pos;
        int block_i = -1;
        for (local_pos.y = 0; local_pos.y < CHUNK_SIZE; local_pos.y++) {
            neighbour_exists[2] = local_pos.y > 0;
            for (local_pos.z = 0; local_pos.z < CHUNK_SIZE; local_pos.z++) {
                neighbour_exists[0] = local_pos.z > 0;
                for (local_pos.x = 0; local_pos.x < CHUNK_SIZE; local_pos.x++) {
                    neighbour_exists[1] = local_pos.x > 0;
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
    size_t nb_translucent_block{0};
    bool should_rebuild_mesh{true};

    Chunk(Chunk&& other) : m_opaque_VAO(other.m_opaque_VAO), m_opaque_VBO(other.m_opaque_VBO), m_opaque_EBO(other.m_opaque_EBO), m_opaque_vertices(other.m_opaque_vertices), m_opaque_triangles(other.m_opaque_triangles),
                           m_translucent_VAO(other.m_translucent_VAO), m_translucent_VBO(other.m_translucent_VBO), m_translucent_EBO(other.m_translucent_EBO), m_translucent_vertices(other.m_translucent_vertices), m_translucent_triangles(other.m_translucent_triangles),
                           m_world(other.m_world), m_pos(other.m_pos), m_blocks(other.m_blocks), m_aabb(other.m_aabb), m_heightmap(other.m_heightmap), m_neighbours(other.m_neighbours), nb_translucent_block(other.nb_translucent_block), should_rebuild_mesh(other.should_rebuild_mesh) {
        for (uint i = 0; i < 6; i++)
            if (m_neighbours[i] != nullptr)
                m_neighbours[i]->m_neighbours[OPPOSITE_FACE[i]] = this;
        other.m_opaque_VAO = other.m_opaque_VBO = other.m_opaque_EBO = other.m_opaque_vertices = other.m_opaque_triangles = other.m_translucent_VAO = other.m_translucent_VBO = other.m_translucent_EBO = other.m_translucent_vertices = 0;
        other.m_world = nullptr;
        other.m_neighbours.fill(nullptr);
    }
    Chunk& operator=(Chunk&& other) {
        if (this == &other)
            return *this;
        clearShaderData(); // free THIS chunk's GPU resources first

        m_world = other.m_world;
        m_pos = other.m_pos;
        m_blocks = other.m_blocks;
        m_aabb = other.m_aabb;
        m_heightmap = other.m_heightmap;
        m_neighbours = other.m_neighbours;
        nb_translucent_block = other.nb_translucent_block;
        should_rebuild_mesh = other.should_rebuild_mesh;
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

        for (uint i = 0; i < 6; i++)
            if (m_neighbours[i] != nullptr)
                m_neighbours[i]->m_neighbours[OPPOSITE_FACE[i]] = this;

        other.m_opaque_VAO = other.m_opaque_VBO = other.m_opaque_EBO = other.m_opaque_vertices = other.m_opaque_triangles = other.m_translucent_VAO = other.m_translucent_VBO = other.m_translucent_EBO = other.m_translucent_vertices = 0;
        other.m_world = nullptr;
        other.m_neighbours.fill(nullptr);
        return *this;
    }
    Chunk(const Chunk&) = delete;
    Chunk& operator=(const Chunk&) = delete;
    ~Chunk() { clearShaderData(); }

    Chunk() : m_world(nullptr), m_neighbours({nullptr}) {}
    Chunk(World* _world, const glm::ivec3& _chunk_pos, GenType _type);

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
    void renderOpaque() const;
    void renderTranslucent() const;
    void clearShaderData();

    friend World;
};

class ChunkStorage {
    static constexpr size_t BATCH_SIZE = (128UL * 1024UL * 1024UL) / sizeof(Chunk); // nombre de chunks qui font 1 GiB
    static_assert(alignof(Chunk) <= alignof(std::max_align_t));                     // if this fails, we need aligned_alloc

    std::vector<std::unique_ptr<std::array<Chunk, BATCH_SIZE>>> m_storage{}; // Ensemble de batch de chunk
    std::vector<std::unique_ptr<std::array<bool, BATCH_SIZE>>> m_alive{};    // Ensemble de batch de "en vie ?"
    MathHelpers::VecMap<int, 3, glm::uvec2> m_lookup_table{};                // chunk pos -> idx de batch et idx de chunk

    std::vector<glm::uvec2> m_free_list{};
    size_t nb_chunks{0};

public:
    ChunkStorage(ChunkStorage&&) = delete;
    ChunkStorage& operator=(ChunkStorage&&) = delete;
    ChunkStorage(const ChunkStorage& other) = delete;
    ChunkStorage& operator=(const ChunkStorage&) = delete;
    ~ChunkStorage() { clear(); }

    ChunkStorage() {}

    inline size_t size() const {
        return nb_chunks;
    }
    inline Chunk* at(const glm::ivec3& _chunk_pos) {
        auto it = m_lookup_table.find(_chunk_pos);
        if (it == m_lookup_table.end())
            return nullptr;
        return &m_storage[it->second.x]->at(it->second.y);
    }
    inline bool isLoaded(const glm::ivec3& _chunk_pos) const {
        return m_lookup_table.find(_chunk_pos) != m_lookup_table.end();
    }

    template <typename... Args>
    glm::uvec2 emplace(Args&&... args) {
        glm::uvec2 indices;
        if (!m_free_list.empty()) {
            indices = m_free_list.back();
            m_free_list.pop_back();
        } else {
            indices.x = nb_chunks / BATCH_SIZE;
            indices.y = nb_chunks - BATCH_SIZE * indices.x;
            if (indices.y == 0) {
                m_storage.push_back(std::make_unique<std::array<Chunk, BATCH_SIZE>>());
                m_alive.push_back(std::make_unique<std::array<bool, BATCH_SIZE>>());
            }
        }

        // Construct directly in place — no move, no copy
        Chunk* slot = &m_storage[indices.x]->at(indices.y);
        std::construct_at(slot, std::forward<Args>(args)...);

        m_alive[indices.x]->at(indices.y) = true;
        m_lookup_table.insert({slot->getPos(), indices});
        nb_chunks += 1;
        return indices;
    }

    void remove(const glm::ivec3& _chunk_pos) {
        glm::uvec2 indices = m_lookup_table[_chunk_pos];
        assert(m_alive[indices.x]->at(indices.y));
        nb_chunks -= 1;
        m_alive[indices.x]->at(indices.y) = false;
        m_free_list.emplace_back(indices);
        m_lookup_table.erase(_chunk_pos);
    }

    template <typename Func>
    inline void forEach(Func fn) {
        for (uint batch_i = 0; batch_i < m_storage.size(); batch_i++)
            for (uint chunk_i = 0; chunk_i < BATCH_SIZE; chunk_i++)
                if (m_alive[batch_i]->at(chunk_i))
                    fn(m_storage[batch_i]->at(chunk_i));
    }
    template <typename Func>
    inline void forEach(Func fn) const {
        for (uint batch_i = 0; batch_i < m_storage.size(); batch_i++)
            for (uint chunk_i = 0; chunk_i < BATCH_SIZE; chunk_i++)
                if (m_alive[batch_i]->at(chunk_i))
                    fn(m_storage[batch_i]->at(chunk_i));
    }

    inline void clear() {
        m_storage.clear();
        m_free_list.clear();
        m_alive.clear();
        nb_chunks = 0;
    }
};