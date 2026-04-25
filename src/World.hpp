#pragma once

// USUAL INCLUDES
#include <map>
#include <set>
#include <list>
#include "Chunk.hpp"
#include "Camera.hpp"
#include "Entity.hpp"

#include <random>
#include <sstream>

class World {
public:
    static constexpr int RENDER_DISTANCE = 5;

private:
    template <typename T, size_t n>
    struct glmVecLexicoGraphic {
        bool operator()(const glm::vec<n, T, glm::packed_highp> &a, const glm::vec<n, T, glm::packed_highp> &b) const {
            return a.x != b.x   ? a.x < b.x
                   : a.y != b.y ? a.y < b.y
                                : a.z < b.z;
        }
    };

    std::map<glm::ivec3, Chunk *, glmVecLexicoGraphic<int, 3>> m_chunks;
    std::set<glm::ivec3, glmVecLexicoGraphic<int, 3>> m_chunks_frontier;
    std::map<std::string, Entity *> m_entities;

public:
    World() {}
    ~World() { clear(); }

    inline bool isChunkLoaded(const glm::ivec3 &_chunk_pos) const { return m_chunks.find(_chunk_pos) != m_chunks.end(); }
    inline bool isChunkFrontier(const glm::ivec3 &_chunk_pos) const { return m_chunks_frontier.find(_chunk_pos) != m_chunks_frontier.end(); }
    Chunk *findChunk(const glm::ivec3 &_chunk_pos);
    Block *findBlock(const glm::ivec3 &_block_pos);
    std::vector<Block *> findSolidBlocks(const glm::ivec3 &start, const glm::ivec3 &end);
    Chunk *addChunk(const glm::ivec3 &_chunk_pos);
    bool removeChunk(const glm::ivec3 &_chunk_pos);
    bool generate(const glm::vec3 &_pos);

    inline bool isEntityLoaded(const std::string &_uuid) const { return m_entities.find(_uuid) != m_entities.end(); }
    Entity *findEntity(const std::string &_uuid);
    Entity *addEntity(Entity::Type _type, const glm::vec3 &_pos);
    bool removeEntity(const std::string &_uuid);
    void update(float _deltaTime);

    // RENDERING

    inline void render(ShaderProgram &_block_shader, ShaderProgram &_line_shader, const Camera &_camera) {
        _block_shader.use();
        _block_shader.set("view", _camera.getViewMatrix());
        _block_shader.set("projection", _camera.getProjectionMatrix());
        _block_shader.set("camera_pos", _camera.m_position);
        _block_shader.set("albedo_atlas", 0);
        _block_shader.set("normal_atlas", 1);
        _block_shader.set("specular_map", 2);

        for (auto &[chunk_pos, chunk] : m_chunks) {
            if (_camera.isVisible(chunk->getAABB())) {
                _block_shader.set("chunk_pos", chunk_pos);
                chunk->render();
            }
        }

        _line_shader.use();
        _line_shader.set("view", _camera.getViewMatrix());
        _line_shader.set("projection", _camera.getProjectionMatrix());
        _line_shader.set("color", glm::vec3(1.f));
        for (auto &[uuid, entity] : m_entities) {
            _line_shader.set("position", entity->m_pos);
            entity->render();
        }
    }
    inline void renderDebugBoxes(ShaderProgram &_line_shader, const Camera &_camera) {
        _line_shader.use();
        _line_shader.set("view", _camera.getViewMatrix());
        _line_shader.set("projection", _camera.getProjectionMatrix());
        _line_shader.set("color", glm::vec3(1.f));
        _line_shader.set("position", glm::vec3(0.f));

        for (auto &[chunk_pos, chunk] : m_chunks) {
            chunk->renderDebugBox();
        }
    }
    inline void clear() {
        for (auto &[chunk_pos, chunk] : m_chunks) {
            delete chunk;
        }
        m_chunks.clear();
        m_chunks_frontier.clear();
    }
};