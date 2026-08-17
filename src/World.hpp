#pragma once

// USUAL INCLUDES
#include <algorithm>
#include <functional>
#include <list>
#include <parsenbt/NBTStruct.hpp>
#include <vector>

#include "Chunk.hpp"
#include "ShaderProgram.hpp"
#include "Window.hpp"

// 28800 = 24 minutes de 60 secondes à 20 ticks par seconde
constexpr uint16_t TICK_SPEED = 20;     // ticks par seconde
constexpr uint64_t DAY_LENGTH = 28800;  // journée en ticks
constexpr uint64_t TIME_SUNRISE = 0;
constexpr uint64_t TIME_NOON = DAY_LENGTH / 4;
constexpr uint64_t TIME_SUNSET = DAY_LENGTH / 2;
constexpr uint64_t TIME_MIDNIGHT = 3 * DAY_LENGTH / 4;
constexpr double TIME_ANGLE_FACTOR = 2 * M_PIf32 / DAY_LENGTH;

enum class Gamerules : uint8_t {
    doDaylightCycle = 0,
    NB_GAMERULES
};

constexpr size_t CHUNK_BATCH_SIZE{16 * 1024 * 1024};  // 16MiB
using ChunkStorage = ContiguousStorage<Chunk, CHUNK_BATCH_SIZE, glm::ivec3, MathHelpers::glmVecLexicoGraphic<int, 3>>;
using ChunkRendererStorage = ContiguousStorage<ChunkRenderer, ChunkStorage::BATCH_SIZE * sizeof(ChunkRenderer), glm::ivec3, MathHelpers::glmVecLexicoGraphic<int, 3>>;

class WorldRenderer;

template <typename vec>
concept glmVec = std::same_as<vec, glm::vec3> ||
                 std::same_as<vec, glm::fvec3> ||
                 std::same_as<vec, glm::dvec3> ||
                 std::same_as<vec, glm::ivec3> ||
                 std::same_as<vec, glm::i8vec3> ||
                 std::same_as<vec, glm::i16vec3> ||
                 std::same_as<vec, glm::i32vec3> ||
                 std::same_as<vec, glm::i64vec3> ||
                 std::same_as<vec, glm::uvec3> ||
                 std::same_as<vec, glm::u8vec3> ||
                 std::same_as<vec, glm::u16vec3> ||
                 std::same_as<vec, glm::u32vec3> ||
                 std::same_as<vec, glm::u64vec3>;

template <glmVec vec, typename T, typename = typename std::enable_if<std::is_arithmetic<T>::value, T>::type>
vec modulo(vec v, T val) {
    return {
        v.x % val,
        v.y % val,
        v.z % val,
    };
}

class World {
   public:
    // Volume of a sphere of radius m_render_distance
    static constexpr size_t getMaxChunkNumber(size_t render_distance) { return 0.75f * M_PIf * render_distance * render_distance * render_distance; }

   private:
    uint m_render_distance{8};
    int m_world_seed{3};
    GenType m_gentype{GenType::SUPERFLAT};
    ChunkStorage m_chunks{};
    MathHelpers::VecSet<int, 3> m_chunks_frontier{};

    bool m_play{true};
    uint64_t m_world_time{TIME_NOON};  // en ticks

    std::list<Chunk*> m_added_chunks;
    std::list<glm::ivec3> m_removed_chunks;
    void generate(const std::vector<glm::vec3>& _centers);

   public:
    int m_floor_height{50}, m_mountain_height{150}, m_water_height{110};

    World(World&&) = delete;
    World& operator=(World&&) = delete;
    World(const World&) = delete;
    World& operator=(const World&) = delete;
    ~World() = default;
    World() {}

    inline const int getWorldSeed() const { return m_world_seed; }
    inline void setWorldSeed(int _seed) {
        m_world_seed = _seed;
    }

    // Chunk functions

    inline bool isChunkFrontier(const glm::ivec3& _chunk_pos) const { return m_chunks_frontier.find(_chunk_pos) != m_chunks_frontier.end(); }
    inline const Chunk* findChunk(const glm::ivec3& _chunk_pos) const { return m_chunks.at(_chunk_pos); }
    const Block* findBlock(const glm::ivec3& _block_pos) const;
    std::vector<const Block*> findBlockLine(const glm::vec3& start, const glm::vec3& end) const;

    inline void reserveChunks() { m_chunks.reserve(World::getMaxChunkNumber(m_render_distance)); }
    inline Chunk* findChunk(const glm::ivec3& _chunk_pos) { return m_chunks.at(_chunk_pos); }
    Block* findBlock(const glm::ivec3& _block_pos);
    Chunk* addChunk(const glm::ivec3& _chunk_pos);
    bool removeChunk(const glm::ivec3& _chunk_pos);

    void placeStructure(const NBTStruct& structure, glm::ivec3 origin_pos) {
        for (const NBT::Block& block : structure.getBlocks()) {
            glm::ivec3 world_pos = block.pos + origin_pos;
            // findBlock(world_pos)->getType() = minecraftTranslation(structure.getBlockName(block));
            findChunk(world_pos)->setBlockType(modulo<glm::ivec3>(world_pos, Chunk::CHUNK_SIZE), minecraftTranslation(structure.getBlockName(block)));
        }
    }

    inline uint64_t& getWorldTime() { return m_world_time; }
    inline GenType getGenType() { return m_gentype; }

    // Update
    void selfUpdate(const std::vector<glm::vec3>& _centers);
    inline void clear() {
        m_chunks.clear();
        m_chunks_frontier.clear();
        m_added_chunks.clear();
        m_removed_chunks.clear();
    }

    // inline void updateEntities(Window& _window, float _dt) { m_ecs_manager.update(_window, _dt); }
    // inline void clearEntities() {
    //     m_ecs_manager.forEachEntity([&](ECS::EntityId entity) { m_ecs_manager.destroyEntity(entity); });
    // }

    friend WorldRenderer;
};