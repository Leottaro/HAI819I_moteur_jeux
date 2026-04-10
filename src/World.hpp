#pragma once

// USUAL INCLUDES
#include "Chunk.hpp"
#include <map>
#include <list>
#include <set>
#include <queue>

class World {
private:
    template <typename T, size_t n>
    struct glmVecLexicoGraphic {
        bool operator()(const glm::vec<n, T, glm::packed_highp> &a, const glm::vec<n, T, glm::packed_highp> &b) const {
            if (a.x != b.x)
                return a.x < b.x;
            if (a.y != b.y)
                return a.y < b.y;
            return a.z < b.z;
        }
    };

    std::map<glm::ivec3, Chunk, glmVecLexicoGraphic<int, 3>> m_chunks;
    std::queue<glm::ivec3> m_chunks_frontier;

public:
    World() {}

    inline bool isChunkLoaded(const glm::ivec3 &_chunk_pos) const { return m_chunks.find(_chunk_pos) != m_chunks.end(); }
    inline Block &getBlock(const glm::ivec3 &_block_pos) { return findChunk(Chunk::blockPosToChunkPos(_block_pos))->getBlock(_block_pos); }

    Chunk *findChunk(const glm::ivec3 &_chunk_pos);
    bool addChunk(const glm::ivec3 &_chunk_pos);
    bool removeChunk(const glm::ivec3 &_chunk_pos);
    bool generate_step();

    inline void render(ShaderProgram &_shader) {
        for (auto &[chunk_pos, chunk] : m_chunks) {
            chunk.render(_shader);
        }
    }
    inline void clear() {
        m_chunks.clear();
        m_chunks_frontier = std::queue<glm::ivec3>();
    }
};