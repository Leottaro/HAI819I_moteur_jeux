#pragma once

// USUAL INCLUDES
#include "Window.hpp"
#include "Chunk.hpp"
#include "ShaderProgram.hpp"

#include <algorithm>
#include <vector>
#include <functional>
#include <list>

// 28800 = 24 minutes de 60 secondes à 20 ticks par seconde
constexpr uint16_t TICK_SPEED = 20;    // ticks par seconde
constexpr uint64_t DAY_LENGTH = 28800; // journée en ticks
constexpr uint64_t TIME_SUNRISE = 0;
constexpr uint64_t TIME_NOON = DAY_LENGTH / 4;
constexpr uint64_t TIME_SUNSET = DAY_LENGTH / 2;
constexpr uint64_t TIME_MIDNIGHT = 3 * DAY_LENGTH / 4;
constexpr double TIME_ANGLE_FACTOR = 2 * M_PIf32 / DAY_LENGTH;

enum class Gamerules : size_t {
    doDaylightCycle = 0,
    NB_GAMERULES
};

constexpr size_t CHUNK_BATCH_SIZE{32 * 1024 * 1024}; // 128MiB
using ChunkStorage = ContiguousStorage<Chunk, CHUNK_BATCH_SIZE, glm::ivec3, MathHelpers::glmVecLexicoGraphic<int, 3>>;
using ChunkRendererStorage = ContiguousStorage<ChunkRenderer, CHUNK_BATCH_SIZE, glm::ivec3, MathHelpers::glmVecLexicoGraphic<int, 3>>;

class WorldRenderer;
class World {
public:
    // Volume of a sphere of radius m_render_distance
    static constexpr size_t getMaxChunkNumber(size_t render_distance) { return 0.75f * M_PIf * render_distance * render_distance * render_distance; }

private:
    uint m_render_distance{12};
    GenType m_gentype{GenType::DEBUG_};
    ChunkStorage m_chunks{getMaxChunkNumber(m_render_distance)};
    MathHelpers::VecSet<int, 3> m_chunks_frontier{};

    bool m_play{true};
    uint64_t m_world_time{TIME_NOON}; // en ticks

    std::list<Chunk*> m_added_chunks;
    std::list<glm::ivec3> m_removed_chunks;
    bool generate(const std::vector<glm::vec3>& _centers);

public:
    World(World&&) = delete;
    World& operator=(World&&) = delete;
    World(const World&) = delete;
    World& operator=(const World&) = delete;
    ~World() = default;
    World() {}

    // Chunk functions

    inline bool isChunkFrontier(const glm::ivec3& _chunk_pos) const { return m_chunks_frontier.find(_chunk_pos) != m_chunks_frontier.end(); }
    inline const Chunk* findChunk(const glm::ivec3& _chunk_pos) const { return m_chunks.at(_chunk_pos); }
    const Block* findBlock(const glm::ivec3& _block_pos) const;
    std::vector<const Block*> findSolidBlocks(const glm::ivec3& start, const glm::ivec3& end) const;

    inline Chunk* findChunk(const glm::ivec3& _chunk_pos) { return m_chunks.at(_chunk_pos); }
    Block* findBlock(const glm::ivec3& _block_pos);
    Chunk* addChunk(const glm::ivec3& _chunk_pos);
    bool removeChunk(const glm::ivec3& _chunk_pos);

    // World time
    inline uint64_t& getWorldTime() { return m_world_time; }

    // ECS manager
    // template <ECS::Component C>
    // inline C& getEntityComponent(ECS::EntityId entity) { return m_ecs_manager.getComponent<C>(entity); }
    // template <ECS::Component C>
    // inline const C& getEntityComponent(ECS::EntityId entity) const { return m_ecs_manager.getComponent<C>(entity); }
    // inline void startControl(Window& _window, ECS::EntityId entity) { m_ecs_manager.startControl(_window, entity); }
    // inline void stopControl(Window& _window) { m_ecs_manager.stopControl(_window); }
    // inline bool hasEntity(ECS::EntityId entity) const { return m_ecs_manager.hasEntity(entity); }
    // inline bool removeEntity(ECS::EntityId entity) { return m_ecs_manager.destroyEntity(entity); }
    // inline ECS::EntityId addTestEntity(const glm::vec3& _pos) { return m_ecs_manager.createEntity<ECS::TestEntity>({ECS::Positionnable{this, _pos}}); }

    // Update
    void update(const std::vector<glm::vec3>& _centers);
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