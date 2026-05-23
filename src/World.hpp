#pragma once

// IMGUI
#include <imgui.h>

// USUAL INCLUDES
#include <algorithm>
#include <functional>
#include <vector>

#include "Chunk.hpp"
#include "ECS/ECS.hpp"

// 28800 = 24 minutes de 60 secondes à 20 ticks par seconde
constexpr uint16_t TICK_SPEED = 20;     // ticks par seconde
constexpr uint64_t DAY_LENGTH = 28800;  // journée en ticks
constexpr uint64_t TIME_SUNRISE = 0;
constexpr uint64_t TIME_NOON = DAY_LENGTH / 4;
constexpr uint64_t TIME_SUNSET = DAY_LENGTH / 2;
constexpr uint64_t TIME_MIDNIGHT = 3 * DAY_LENGTH / 4;
constexpr double TIME_ANGLE_FACTOR = 2 * M_PIf32 / DAY_LENGTH;

// DISPLAY CONST
constexpr glm::vec4 SKY_DAY(187.f / 255.f, 255.f / 255.f, 250.f / 255.f, 255.f / 255.f);
constexpr glm::vec4 SKY_NIGHT(14.f / 255.f, 5.f / 255.f, 61.f / 255.f, 255.f / 255.f);
constexpr glm::vec3 SUN_NOON(209.f / 255.f, 209.f / 255.f, 175.f / 255.f);
// constexpr glm::vec3 SUN_NOON(0.f / 255.f, 0.f / 255.f, 175.f / 255.f);
constexpr glm::vec3 SUN_DUSK(255.f / 255.f, 167.f / 255.f, 41.f / 255.f);

enum class Gamerules : size_t {
    doDaylightCycle = 0,
    NB_GAMERULES
};

constexpr size_t CHUNK_BATCH_SIZE{128 * 1024 * 1024};
using ChunkStorage = ContiguousStorage<Chunk, CHUNK_BATCH_SIZE, glm::ivec3, MathHelpers::glmVecLexicoGraphic<int, 3>>;
using ChunkRendererStorage = ContiguousStorage<ChunkRenderer, CHUNK_BATCH_SIZE, glm::ivec3, MathHelpers::glmVecLexicoGraphic<int, 3>>;

class WorldRenderer;
class World {
    int m_world_seed = 0;
    int m_render_distance{4};
    GenType m_gentype{GenType::DEBUG_};
    ChunkStorage m_chunks{};
    MathHelpers::VecSet<int, 3> m_chunks_frontier{};
    ECSManager m_ecs_manager{};

    double m_last_update{};
    bool m_time_ff{true};
    bool m_play{true};
    double m_tick_accumulator{0.};

    uint64_t m_world_time{TIME_NOON};

    bool generate(const std::vector<glm::vec3>& _positions);

   public:
    std::function<void(Chunk*)> onAddChunk{nullptr};
    std::function<void(const glm::ivec3&)> onRemoveChunk{nullptr};

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
    Chunk* findChunk(const glm::ivec3& _chunk_pos);
    Block* findBlock(const glm::ivec3& _block_pos);
    std::vector<const Block*> findSolidBlocks(const glm::ivec3& start, const glm::ivec3& end);
    Chunk* addChunk(const glm::ivec3& _chunk_pos);
    bool removeChunk(const glm::ivec3& _chunk_pos);

    // ECS manager
    template <ECS::Component C>
    inline C& getEntityComponent(ECS::EntityId entity) { return m_ecs_manager.getComponent<C>(entity); }
    template <ECS::Component C>
    inline const C& getEntityComponent(ECS::EntityId entity) const { return m_ecs_manager.getComponent<C>(entity); }
    inline void startControl(Window& _window, ECS::EntityId entity) { m_ecs_manager.startControl(_window, entity); }
    inline void stopControl(Window& _window) { m_ecs_manager.stopControl(_window); }
    inline bool hasEntity(ECS::EntityId entity) const { return m_ecs_manager.hasEntity(entity); }
    inline bool removeEntity(ECS::EntityId entity) { return m_ecs_manager.destroyEntity(entity); }
    inline ECS::EntityId addTestEntity(const glm::vec3& _pos) { return m_ecs_manager.createEntity<ECS::TestEntity>({ECS::Positionnable{this, _pos}}); }

    // World time
    inline uint64_t& getWorldTime() { return m_world_time; }
    inline void play() { m_play = m_time_ff = true; }
    inline void pause() { m_play = false; }
    inline void toggleTime() { m_play ? pause() : play(); }

    // Update
    bool updateTime();
    void updateChunks();
    inline void clearChunks() { m_chunks.clear(); }

    inline void updateEntities(Window& _window, float _dt) { m_ecs_manager.update(_window, _dt); }
    inline void clearEntities() {
        m_ecs_manager.forEachEntity([&](ECS::EntityId entity) { m_ecs_manager.destroyEntity(entity); });
    }

    friend WorldRenderer;
};

class WorldRenderer {
    World* m_world{nullptr};
    ChunkRendererStorage m_render_chunks{};
    glm::ivec3 m_last_cam_chunk{Chunk::CHUNK_SIZE + 1};  // Initialisé a un chunk impossible

    float m_sun_season{0.f};

   public:
    WorldRenderer(WorldRenderer&& other) = delete;
    WorldRenderer& operator=(WorldRenderer&& other) = delete;
    WorldRenderer(const WorldRenderer& other) = delete;
    WorldRenderer& operator=(const WorldRenderer& other) = delete;
    ~WorldRenderer() {
        m_world->onAddChunk = nullptr;
        m_world->onRemoveChunk = nullptr;
        m_world = nullptr;
        m_render_chunks.clear();
    }

    WorldRenderer() {}
    WorldRenderer(World* _world) { setWorld(_world); }

    void setWorld(World* _world);
    void clear();

    glm::vec3 skyColor() const;
    glm::vec3 sunColor() const;

    void renderChunks(ShaderProgram& _chunk_shader);
    void renderDebugBoxes(ShaderProgram& _line_shader) const;
    void renderEntities(ShaderProgram& _line_shader) const;

    inline void updateInterface(Window& _window) {
        if (m_world == nullptr || !ImGui::Begin("World Info")) {
            ImGui::End();
            return;
        }

        m_world->m_ecs_manager.updateInterfaces(_window);

        int current_type = static_cast<int>(m_world->m_gentype);
        if (ImGui::Combo("Generation Type", &current_type, GenTypeNames, IM_ARRAYSIZE(GenTypeNames))) {
            m_world->m_gentype = static_cast<GenType>(current_type);
            m_render_chunks.clear();
            m_world->clearChunks();
            m_world->updateChunks();
        }

        if (ImGui::InputInt("Render distance", &m_world->m_render_distance, 1, 2)) {
            m_world->m_render_distance = std::max(m_world->m_render_distance, 1);
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::DragScalar("World Time", ImGuiDataType_U64, &m_world->m_world_time, 10.0f, 0, &DAY_LENGTH)) {
            m_world->updateTime();
        }
        if (ImGui::DragFloat("World Season", &m_sun_season, 0.01f, -M_2_PIf, M_2_PIf)) {
            m_sun_season = std::clamp(m_sun_season, -M_2_PIf, M_2_PIf);
        }
        if (ImGui::Button(m_world->m_play ? "Pause" : "Play")) {
            m_world->toggleTime();
        }
        ImGui::Text("Seed actuelle : %d", m_world->getWorldSeed());


        static char new_seed[64];
        ImGui::InputText("New Seed", new_seed, sizeof(new_seed));
        ImGui::SameLine();
        if (ImGui::Button("Regen")) {
            int new_seed_val;
            std::string_view new_seed_str(new_seed);
            std::cout << "current string : " << new_seed << "\n";

            // Trying to parse string to int
            auto [ptr, ec] = std::from_chars(
                new_seed_str.data(),
                new_seed_str.data() + new_seed_str.size(),
                new_seed_val);

            // If not possible, hashing string to int
            if (ec != std::errc() || ptr != new_seed_str.data() + new_seed_str.size()) {
                // https://stackoverflow.com/questions/16075271/hashing-a-string-to-an-integer-in-c
                std::cout << new_seed_str << " is not a number, hashing...\n";
                new_seed_val = std::hash<std::string_view>{}(new_seed_str);
            }

            std::cout << new_seed_val << '\n';
            m_world->setWorldSeed(new_seed_val);
            m_render_chunks.clear();
            m_world->clearChunks();
        }

        ImGui::End();
    }
};