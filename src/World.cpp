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
    m_chunks_frontier.erase(_chunk_pos);
    Chunk &inserted_chunk = m_chunks.at(_chunk_pos);

    // on ajoute tous ses voisins dans la frontière si ils ne sont pas déjà chargé sinon on update le chunk car il a un nouveau voisin !
    for (int face_i = 0; face_i < 6; face_i++) {
        glm::ivec3 neighbour_pos = _chunk_pos + Chunk::NEIGHBOURS_POS[face_i];
        Chunk *neighbour = findChunk(neighbour_pos);
        if (neighbour == nullptr) {
            m_chunks_frontier.insert(neighbour_pos);
        } else {
            inserted_chunk.m_neighbours[face_i] = neighbour;
            neighbour->m_neighbours[OPPOSITE_FACE[face_i]] = &inserted_chunk;
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

    for (int face_i = 0; face_i < 6; face_i++) {
        glm::ivec3 neighbour_pos = _chunk_pos + Chunk::NEIGHBOURS_POS[face_i];
        Chunk *neighbour = findChunk(neighbour_pos);
        if (neighbour != nullptr) {
            // neighbour chunk is loaded, we update it and put the current chunk in the frontier
            m_chunks_frontier.insert(_chunk_pos);
            neighbour->m_neighbours[OPPOSITE_FACE[face_i]] = nullptr;
            neighbour->recomputeBlockNeighbours();
            neighbour->buildMesh();
        } else if (isChunkFrontier(neighbour_pos)) {
            // neighbour chunk is in frontier, remove it if it has no loaded neighbour.
            bool neighbour_has_neighbour = false;
            for (int neighbour_face_i = 0; neighbour_face_i < 6; neighbour_face_i++) {
                glm::ivec3 neighbour_neighbour_pos = neighbour_pos + Chunk::NEIGHBOURS_POS[face_i];
                if (isChunkLoaded(neighbour_neighbour_pos)) {
                    neighbour_has_neighbour = true;
                    break;
                }
            }
            if (!neighbour_has_neighbour) {
                m_chunks_frontier.erase(neighbour_pos);
            }
        }
    }

    return true;
}

bool World::generate_step() {
    // glm::ivec3 next_chunk;
    // do {
    //     if (m_chunks_frontier.empty())
    //         return false;
    //     next_chunk = m_chunks_frontier.back();
    //     m_chunks_frontier.pop_back();
    // } while (isChunkLoaded(next_chunk));
    // addChunk(next_chunk);
    return true;
}
