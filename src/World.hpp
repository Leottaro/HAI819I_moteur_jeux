#pragma once

// USUAL INCLUDES
#include "Chunk.hpp"
#include "ECS/ECS.hpp"
#include <map>
#include <set>
#include <list>
#include <imgui.h>
#include <random>
#include <set>
#include <sstream>
#include <vector>

// 28800 = 24 minutes de 60 secondes à 20 ticks par seconde
constexpr uint16_t TICK_SPEED = 20;    // ticks par seconde
constexpr uint64_t DAY_LENGTH = 28800; // journée en ticks
constexpr uint64_t TIME_SUNRISE = 0;
constexpr uint64_t TIME_NOON = DAY_LENGTH / 4;
constexpr uint64_t TIME_SUNSET = DAY_LENGTH / 2;
constexpr uint64_t TIME_MIDNIGHT = 3 * DAY_LENGTH / 4;
constexpr double TIME_ANGLE_FACTOR = 2 * M_PIf32 / DAY_LENGTH;

class World {
public:
    static constexpr int RENDER_DISTANCE = 5;

private:
    template <typename T, size_t n>
    struct glmVecLexicoGraphic {
        bool operator()(const glm::vec<n, T, glm::packed_highp>& a, const glm::vec<n, T, glm::packed_highp>& b) const {
            return a.x != b.x   ? a.x < b.x
                   : a.y != b.y ? a.y < b.y
                                : a.z < b.z;
        }
    };

    std::map<glm::ivec3, Chunk*, glmVecLexicoGraphic<int, 3>> m_chunks;
    std::set<glm::ivec3, glmVecLexicoGraphic<int, 3>> m_chunks_frontier;
    ECSManager m_ecs_manager;

    double m_last_update = glfwGetTime();
    bool m_time_ff = true;
    bool m_play = true;
    double m_tick_accumulator = 0.f;

    uint64_t m_world_time = TIME_NOON;
    float m_sun_season = 0.f;
    glm::fvec2 m_sun_pos = glm::fvec2(m_sun_season, TIME_NOON* TIME_ANGLE_FACTOR); // position du soleil (nord<->sud, est<->ouest)

    GenType m_gentype = GenType::DEBUG_;

    inline ECS::Positionnable ECSPosition(const glm::vec3& _pos) {
        return ECS::Positionnable{findChunk(Chunk::posToChunkPos(_pos)), _pos};
    }

public:
    World() {}
    ~World() { clear(); }

    inline bool isChunkLoaded(const glm::ivec3& _chunk_pos) const { return m_chunks.find(_chunk_pos) != m_chunks.end(); }
    inline bool isChunkFrontier(const glm::ivec3& _chunk_pos) const { return m_chunks_frontier.find(_chunk_pos) != m_chunks_frontier.end(); }
    Chunk* findChunk(const glm::ivec3& _chunk_pos);
    Block* findBlock(const glm::ivec3& _block_pos);
    std::vector<Block*> findSolidBlocks(const glm::ivec3& start, const glm::ivec3& end);
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
    inline void updateEntities(Window& _window) { m_ecs_manager.update(_window, 0.01); } // TODO: dt
    inline void renderEntities(ShaderProgram& _line_shader) { m_ecs_manager.render(_line_shader); }
    ECS::EntityId addTestEntity(const glm::vec3& _pos);

    // World time

    inline uint64_t* getWorldTime() { return &m_world_time; }
    inline void play() { m_play = m_time_ff = true; }
    inline void pause() { m_play = false; }
    inline void toggleTime() { m_play ? pause() : play(); }

    // RENDERING

    void renderChunks(ShaderProgram& _block_shader) {
        glm::vec4 sky_color = skyColor();
        glClearColor(sky_color.r, sky_color.g, sky_color.b, sky_color.a);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

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
            }
        }

        ECS::CamerableSystem& camerable_system = m_ecs_manager.getSystem<ECS::CamerableSystem>();

        _block_shader.use();
        _block_shader.set("view", camerable_system.getView());
        _block_shader.set("projection", camerable_system.getProjection());
        _block_shader.set("camera_pos", camerable_system.getCamPos());
        _block_shader.set("sun_pos", m_sun_pos);
        _block_shader.set("albedo_atlas", 0);
        _block_shader.set("normal_atlas", 1);
        _block_shader.set("specular_atlas", 2);

        for (auto& [chunk_pos, chunk] : m_chunks) {
            // if (_camera.isVisible(chunk->getAABB())) { // TODO:
            _block_shader.set("chunk_pos", chunk_pos);
            chunk->render();
            // }
        }
    }
    void renderDebugBoxes(ShaderProgram& _line_shader) {
        ECS::CamerableSystem& camerable_system = m_ecs_manager.getSystem<ECS::CamerableSystem>();
        _line_shader.use();
        _line_shader.set("view", camerable_system.getView());
        _line_shader.set("projection", camerable_system.getProjection());
        _line_shader.set("color", glm::vec3(1.f));
        _line_shader.set("position", glm::vec3(0.f));

        for (auto& [chunk_pos, chunk] : m_chunks) {
            chunk->renderDebugBox();
        }
    }
    inline void clear() {
        for (auto& [chunk_pos, chunk] : m_chunks) {
            delete chunk;
        }
        m_chunks.clear();
        m_chunks_frontier.clear();
    }

    inline glm::vec4 skyColor() const {
        glm::vec4 SKY_DAY(187.f, 255.f, 250.f, 255.f);
        SKY_DAY /= 255.f;
        glm::vec4 SKY_NIGHT(14.f, 5.f, 61.f, 255.f);
        SKY_NIGHT /= 255.f;
        float daylight_factor = sin(m_world_time * M_PI * 2 / DAY_LENGTH);
        return daylight_factor * SKY_DAY + (1 - daylight_factor) * SKY_NIGHT;
    }

    void updateWindow() {
        if (ImGui::Begin("World Info")) {
            int current_type = static_cast<int>(m_gentype);
            if (ImGui::Combo("Generation Type", &current_type,
                             GenTypeNames,
                             IM_ARRAYSIZE(GenTypeNames))) {
                m_gentype = static_cast<GenType>(current_type);
                clear();
                generate();
            }
            if (ImGui::DragScalar(
                    "World Time",
                    ImGuiDataType_U64,
                    &m_world_time,
                    10.0f,
                    0,
                    &DAY_LENGTH)) {
                // render(_block_shader, _camera);
            }
            ImGui::DragFloat(
                "World Season",
                &m_sun_season,
                0.01f,
                -M_2_PI,
                M_2_PI);
            if (ImGui::Button(m_play ? "Pause" : "Play"))
                toggleTime();
        }
        ImGui::End();
    }
};