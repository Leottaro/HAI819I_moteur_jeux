#pragma once

// USUAL INCLUDES
#include "Chunk.hpp"
#include "Camera.hpp"
#include <map>
#include <set>

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

public:
    World() {}
    ~World() { clear(); }

    inline bool isChunkLoaded(const glm::ivec3 &_chunk_pos) const { return m_chunks.find(_chunk_pos) != m_chunks.end(); }
    inline bool isChunkFrontier(const glm::ivec3 &_chunk_pos) const { return m_chunks_frontier.find(_chunk_pos) != m_chunks_frontier.end(); }
    inline Block &getBlock(const glm::ivec3 &_block_pos) { return findChunk(Chunk::blockPosToChunkPos(_block_pos))->getBlock(_block_pos); }

    Chunk *findChunk(const glm::ivec3 &_chunk_pos);
    bool addChunk(const glm::ivec3 &_chunk_pos);
    bool removeChunk(const glm::ivec3 &_chunk_pos);
    bool generate(const glm::vec3 &_pos);

    // RENDERING

    inline void render(ShaderProgram &_block_shader, const Camera &_camera) {
        _block_shader.set("view", _camera.getViewMatrix());
        _block_shader.set("projection", _camera.getProjectionMatrix());
        _block_shader.set("block_atlas", 0);

        for (auto &[chunk_pos, chunk] : m_chunks) {
            // if (_camera.isVisible(chunk->getAABB())) {
            chunk->render();
            // }
        }
    }
    inline void renderDebugBoxes(ShaderProgram &_line_shader, const Camera &_camera) {
        _line_shader.set("view", _camera.getViewMatrix());
        _line_shader.set("projection", _camera.getProjectionMatrix());
        _line_shader.set("color", glm::vec3(1.f));

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