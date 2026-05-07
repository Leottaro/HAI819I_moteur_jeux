#pragma once

// USUAL INCLUDES
#include "Camera.hpp"
#include "Chunk.hpp"
#include "ECS/ECS.hpp"
#include <map>
#include <set>
#include <list>
#include <imgui.h>
#include <random>
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

    Chunk::GenType m_gentype = Chunk::GenType::DEBUG_;

    inline ECS::Position ECSPosition(const glm::vec3& _pos) {
        return ECS::Position{findChunk(Block::posToBlockPos(_pos)), _pos};
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

    // ECS manager
    template <ECS::Component C>
    inline C& getEntityComponent(ECS::EntityId entity) { return m_ecs_manager.getComponent<C>(entity); }

    inline bool hasEntity(ECS::EntityId entity) { return m_ecs_manager.hasEntity(entity); }
    inline bool removeEntity(ECS::EntityId entity) { return m_ecs_manager.destroyEntity(entity); }
    inline void updateEntities(float _deltaTime) { m_ecs_manager.update(_deltaTime); }
    inline void renderEntities(const Camera& _camera, ShaderProgram& _line_shader) { m_ecs_manager.render(_camera, _line_shader); }
    ECS::EntityId addTestEntity(const glm::vec3& _pos);

    inline uint64_t* getWorldTime() { return &m_world_time; }
    inline void play() { m_play = m_time_ff = true; }
    inline void pause() { m_play = false; }
    inline void toggleTime() { m_play ? pause() : play(); }

    // RENDERING

    inline void renderChunks(const Camera& _camera, ShaderProgram& _block_shader) {
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

        _block_shader.use();
        _block_shader.set("view", _camera.getViewMatrix());
        _block_shader.set("projection", _camera.getProjectionMatrix());
        _block_shader.set("camera_pos", _camera.m_position);
        _block_shader.set("sun_pos", m_sun_pos);
        _block_shader.set("albedo_atlas", 0);
        _block_shader.set("normal_atlas", 1);
        _block_shader.set("specular_atlas", 2);

        for (auto& [chunk_pos, chunk] : m_chunks) {
            if (_camera.isVisible(chunk->getAABB())) {
                _block_shader.set("chunk_pos", chunk_pos);
                chunk->render();
            }
        }
    }
    inline void renderDebugBoxes(ShaderProgram& _line_shader, const Camera& _camera) {
        _line_shader.use();
        _line_shader.set("view", _camera.getViewMatrix());
        _line_shader.set("projection", _camera.getProjectionMatrix());
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

    void updateWindow(Camera _camera) {
        if (ImGui::Begin("World Info")) {
            int current_type = static_cast<int>(m_gentype);
            if (ImGui::Combo("Generation Type", &current_type, "Debug\0Superflat\0")) {
                m_gentype = Chunk::GenType(current_type);
                clear();
                generate(_camera.getFront());
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