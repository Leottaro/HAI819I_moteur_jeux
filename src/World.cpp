// USUAL INCLUDES
#include "World.hpp"

Chunk *World::findChunk(const glm::ivec3 &_chunk_pos) {
    if (!isChunkLoaded(_chunk_pos)) {
        for (Chunk &chunk : m_chunks) {
            if (chunk.getPos() == _chunk_pos) {
                return &chunk;
            }
        }
    }
    return nullptr;
}

bool World::addChunk(const glm::ivec3 &_chunk_pos) {
    if (!m_loaded_chunks.insert(_chunk_pos).second) {
        return false;
    }
    m_chunks.push_back(Chunk(_chunk_pos, Chunk::GenType::SUPERFLAT));
    m_chunks.back().buildMesh();

    // on ajoute tous ses voisins dans la frontière (si ils ne sont pas déjà chargé)
    for (const glm::ivec3 &_chunk_offset : Chunk::NEIGHBOURS_POS) {
        glm::ivec3 neighbour = _chunk_pos + _chunk_offset;
        if (!isChunkLoaded(neighbour)) {
            m_chunks_frontier.push(neighbour);
        }
    }

    return true;
}
bool World::removeChunk(const glm::ivec3 &_chunk_pos) {
    if (m_loaded_chunks.erase(_chunk_pos) == 0)
        return false;

    // on le remets dans la frontière si un de ses voisins est chargé
    for (const glm::ivec3 &_chunk_offset : Chunk::NEIGHBOURS_POS) {
        glm::ivec3 neighbour = _chunk_pos + _chunk_offset;
        if (isChunkLoaded(neighbour)) {
            m_chunks_frontier.push(_chunk_pos);
        }
    }

    return true;
}

bool World::generate_step() {
    glm::ivec3 next_chunk;
    do {
        if (m_chunks_frontier.empty())
            return false;
        next_chunk = m_chunks_frontier.front();
        m_chunks_frontier.pop();
    } while (isChunkLoaded(next_chunk));
    addChunk(next_chunk);
    return true;
}
