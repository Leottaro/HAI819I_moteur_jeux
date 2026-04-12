#pragma once

// USUAL INCLUDES
#include "Chunk.hpp"
#include <map>
#include <set>

class World {
public:
    static constexpr int RENDER_DISTANCE = 8;

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

    inline void render(ShaderProgram &_shader) {
        for (auto &[chunk_pos, chunk] : m_chunks) {
            chunk->render(_shader);
        }
    }
    inline void renderDebugBoxes(ShaderProgram &_shader) {
        for (auto &[chunk_pos, chunk] : m_chunks) {
            chunk->renderDebugBox(_shader);
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