#pragma once

// IMGUI
#include <imgui.h>

// USUAL INCLUDES
#include "ECS/ECS.hpp"
#include "Chunk.hpp"

#include <algorithm>
#include <vector>

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

constexpr glm::vec4 SKY_DAY(187.f / 255.f, 255.f / 255.f, 250.f / 255.f, 255.f / 255.f);
constexpr glm::vec4 SKY_NIGHT(14.f / 255.f, 5.f / 255.f, 61.f / 255.f, 255.f / 255.f);
constexpr glm::vec3 SUN_NOON(209.f / 255.f, 209.f / 255.f, 175.f / 255.f);
// constexpr glm::vec3 SUN_NOON(0.f / 255.f, 0.f / 255.f, 175.f / 255.f);
constexpr glm::vec3 SUN_DUSK(255.f / 255.f, 167.f / 255.f, 41.f / 255.f);

class World {
    ChunkStorage m_chunks{};
    MathHelpers::VecSet<int, 3> m_chunks_frontier{};
    ECSManager m_ecs_manager{};

    double m_last_update{};
    bool m_time_ff{true};
    bool m_play{true};
    double m_tick_accumulator{0.};

    uint64_t m_world_time{TIME_NOON};
    float m_sun_season{0.f};
    glm::fvec2 m_sun_pos{m_sun_season, TIME_NOON * TIME_ANGLE_FACTOR}; // position du soleil (nord<->sud, est<->ouest)
    glm::vec3 m_sun_color{SUN_NOON};

    int m_render_distance{2};
    GenType m_gentype{GenType::DEBUG_};

public:
    World(World&&) = delete;
    World& operator=(World&&) = delete;
    World(const World&) = delete;
    World& operator=(const World&) = delete;
    ~World() { clear(); }
    World() {}

    inline bool isChunkLoaded(const glm::ivec3& _chunk_pos) const { return m_chunks.isLoaded(_chunk_pos); }
    inline bool isChunkFrontier(const glm::ivec3& _chunk_pos) const { return m_chunks_frontier.find(_chunk_pos) != m_chunks_frontier.end(); }
    Chunk* findChunk(const glm::ivec3& _chunk_pos);
    Block* findBlock(const glm::ivec3& _block_pos);
    std::vector<const Block*> findSolidBlocks(const glm::ivec3& start, const glm::ivec3& end);
    Chunk* addChunk(const glm::ivec3& _chunk_pos);
    bool removeChunk(const glm::ivec3& _chunk_pos);
    bool generate(const glm::vec3& _pos);
    inline bool generate() { return generate(m_ecs_manager.getSystem<ECS::CamerableSystem>().getCamPos()); } // TODO: for each controlled entities

    // ECS manager
    template <ECS::Component C>
    inline C& getEntityComponent(ECS::EntityId entity) { return m_ecs_manager.getComponent<C>(entity); }
    inline void startControl(Window& _window, ECS::EntityId entity) { m_ecs_manager.startControl(_window, entity); }
    inline void stopControl(Window& _window) { m_ecs_manager.stopControl(_window); }

    inline bool hasEntity(ECS::EntityId entity) { return m_ecs_manager.hasEntity(entity); }
    inline bool removeEntity(ECS::EntityId entity) { return m_ecs_manager.destroyEntity(entity); }
    inline void updateEntities(Window& _window, float _dt) { m_ecs_manager.update(_window, _dt); }
    inline void renderEntities(ShaderProgram& _line_shader) { m_ecs_manager.render(_line_shader); }
    ECS::EntityId addTestEntity(const glm::vec3& _pos);

    // World time

    inline uint64_t* getWorldTime() { return &m_world_time; }
    inline void play() { m_play = m_time_ff = true; }
    inline void pause() { m_play = false; }
    inline void toggleTime() { m_play ? pause() : play(); }

    // RENDERING

    void updateTime() {
        if (m_play) {
            if (m_time_ff) {
                m_last_update = glfwGetTime();
                m_time_ff = false;
            } else {
                double current_time = glfwGetTime();                               // in seconds, capture
                double delta_time = current_time - m_last_update;                  // in seconds, computing delta time
                m_last_update = current_time;                                      // update for next step
                double delta_ticks = delta_time * TICK_SPEED;                      // seconds * ticks / seconds = ticks
                m_tick_accumulator += delta_ticks;                                 // save to avoir drifting
                uint64_t ticks_passed = static_cast<uint64_t>(m_tick_accumulator); // seconds -> ticks
                m_tick_accumulator -= ticks_passed;                                // reset for next delta
                m_world_time = (m_world_time + ticks_passed) % DAY_LENGTH;         // in ticks % ticks per day (time changed)

                m_sun_pos.x = m_sun_season;
                m_sun_pos.y = m_world_time * TIME_ANGLE_FACTOR;
                m_sun_color = sunColor();
            }
        }
    }

    glm::ivec3 last_cam_chunk{Chunk::CHUNK_SIZE - 1}; // Initialisé a un chunk impossible
    void renderChunks(ShaderProgram& _block_shader) {
        updateTime();

        ECS::CamerableSystem& camerable_system = m_ecs_manager.getSystem<ECS::CamerableSystem>();

        _block_shader.use();
        _block_shader.set("view", camerable_system.getView());
        _block_shader.set("projection", camerable_system.getProjection());
        _block_shader.set("camera_pos", camerable_system.getCamPos());
        _block_shader.set("sun_pos", m_sun_pos);
        _block_shader.set("sun_color", m_sun_color);
        _block_shader.set("albedo_atlas", 0);
        _block_shader.set("normal_atlas", 1);
        _block_shader.set("specular_atlas", 2);

        glm::ivec3 cam_chunk = Chunk::posToChunkPos(camerable_system.getCamPos());
        bool rebuild_all_meshes = cam_chunk != last_cam_chunk;
        std::vector<std::pair<float, Chunk*>> drawed_chunks;
        drawed_chunks.reserve(m_chunks.size());
        m_chunks.forEach([&](Chunk& chunk) {
            if (rebuild_all_meshes)
                chunk.should_rebuild_mesh = true;

            if (camerable_system.getFrustum().isVisible(chunk.getAABB())) {
                float dist = glm::distance(camerable_system.getCamPos(), glm::vec3(chunk.getPos()) + glm::vec3(Chunk::CHUNK_SIZE * 0.5f));
                if (cam_chunk.x == chunk.getPos().x || cam_chunk.y == chunk.getPos().y || cam_chunk.z == chunk.getPos().z || chunk.should_rebuild_mesh) {
                    chunk.updateShaderData(camerable_system.getCamPos());
                }
                drawed_chunks.push_back({dist, &chunk});
            }
        });
        last_cam_chunk = cam_chunk;
        std::sort(drawed_chunks.begin(), drawed_chunks.end(), [](auto& a, auto& b) { return a.first > b.first; }); // On sort par distance décroissante

        // https://claude.ai/share/8b8db085-496a-4a82-a253-38586a504c3c
        for (auto& [_, chunk] : drawed_chunks) {
            _block_shader.set("chunk_pos", chunk->getPos());
            chunk->renderOpaque();
        }
        glDepthMask(GL_FALSE);
        glEnable(GL_BLEND);
        for (auto& [_, chunk] : drawed_chunks) {
            _block_shader.set("chunk_pos", chunk->getPos());
            chunk->renderTranslucent();
        }
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
    }
    void renderDebugBoxes(ShaderProgram& _line_shader) {
        ECS::CamerableSystem& camerable_system = m_ecs_manager.getSystem<ECS::CamerableSystem>();
        _line_shader.use();
        _line_shader.set("view", camerable_system.getView());
        _line_shader.set("projection", camerable_system.getProjection());
        _line_shader.set("color", glm::vec3(1.f));
        _line_shader.set("position", glm::vec3(0.f));

        AABB<float> chunk_hitbox(glm::vec3(0), glm::vec3(Chunk::CHUNK_SIZE));
        AABBRenderer box(chunk_hitbox);
        m_chunks.forEach([&_line_shader, &box](Chunk& chunk) {
            _line_shader.set("position", glm::vec3(chunk.getPos()));
            box.render();
        });
        box.clearShaderData();

        chunk_hitbox.min = glm::vec3(Chunk::CHUNK_SIZE / 4);
        chunk_hitbox.max = glm::vec3(3 * Chunk::CHUNK_SIZE / 4);
        box.initShaderData(chunk_hitbox);
        for (const glm::ivec3& chunk_pos : m_chunks_frontier) {
            _line_shader.set("position", glm::vec3(chunk_pos));
            box.render();
        }
        box.clearShaderData();
    }
    inline void clear() {
        m_chunks.clear();
        m_chunks_frontier.clear();
    }

    inline glm::vec3 skyColor() const {
        double angle = m_world_time * M_PI * 2.0 / DAY_LENGTH;
        float t = (sin(angle) + 1.0f) * 0.5f;
        return glm::mix(SKY_NIGHT, SKY_DAY, t);
    }

    inline glm::vec3 sunColor() const {
        double angle = (m_world_time * M_PI * 4.0 / DAY_LENGTH) + M_PI_2;
        float t = (sin(angle) + 1.0f) * 0.5f;
        return glm::mix(SUN_NOON, SUN_DUSK, t);
    }

    inline void updateWindow() {
        if (ImGui::Begin("World Info")) {
            int current_type = static_cast<int>(m_gentype);
            if (ImGui::Combo("Generation Type", &current_type, GenTypeNames, IM_ARRAYSIZE(GenTypeNames))) {
                m_gentype = static_cast<GenType>(current_type);
                clear();
                generate();
            }

            if (ImGui::InputInt("Render distance", &m_render_distance, 1, 2)) {
                m_render_distance = std::max(m_render_distance, 1);
            }

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            if (ImGui::DragScalar("World Time", ImGuiDataType_U64, &m_world_time, 10.0f, 0, &DAY_LENGTH)) {
                updateTime();
            }
            if (ImGui::DragFloat("World Season", &m_sun_season, 0.01f, -M_2_PIf, M_2_PIf)) {
                m_sun_season = std::clamp(m_sun_season, -M_2_PIf, M_2_PIf);
            }
            if (ImGui::Button(m_play ? "Pause" : "Play")) {
                toggleTime();
            }
        }
        ImGui::End();
    }
};