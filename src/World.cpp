// USUAL INCLUDES
#include "World.hpp"

Chunk *World::findChunk(const glm::ivec3 &_chunk_pos) {
    if (isChunkLoaded(_chunk_pos)) {
        return &m_chunks.at(_chunk_pos);
    }
    return nullptr;
}

bool World::addChunk(const glm::ivec3 &_chunk_pos) {
    if (!m_chunks.insert({_chunk_pos, Chunk(_chunk_pos, Chunk::GenType::SUPERFLAT)}).second) {
        return false;
    }
    Chunk &inserted_chunk = m_chunks.at(_chunk_pos);

    // on ajoute tous ses voisins dans la frontière si ils ne sont pas déjà chargé sinon on update le chunk car il a un nouveau voisin !
    for (int face_i = 0; face_i < 6; face_i++) {
        glm::ivec3 neighbour_pos = _chunk_pos + Chunk::NEIGHBOURS_POS[face_i];
        Chunk *neighbour = findChunk(neighbour_pos);
        if (neighbour == nullptr) {
            m_chunks_frontier.push(neighbour_pos);
        } else {
            inserted_chunk.m_neighbours[face_i] = neighbour;
            neighbour->m_neighbours[(face_i + 3) % 6] = &inserted_chunk;
            neighbour->recomputeBlockNeighbours();
            neighbour->buildMesh();
        }
    }

    inserted_chunk.recomputeBlockNeighbours();
    inserted_chunk.buildMesh();

    return true;
}
bool World::removeChunk(const glm::ivec3 &_chunk_pos) {
    if (m_chunks.erase(_chunk_pos) == 0)
        return false;

    // on le remets dans la frontière si un de ses voisins est chargé
    for (int face_i = 0; face_i < 6; face_i++) {
        glm::ivec3 neighbour_pos = _chunk_pos + Chunk::NEIGHBOURS_POS[face_i];
        Chunk *neighbour = findChunk(neighbour_pos);
        if (neighbour != nullptr) {
            m_chunks_frontier.push(_chunk_pos);
            neighbour->m_neighbours[(face_i + 3) % 6] = nullptr;
            neighbour->recomputeBlockNeighbours();
            neighbour->buildMesh();
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
