// USUAL INCLUDES
#include "World.hpp"
#include <list>

Chunk *World::findChunk(const glm::ivec3 &_chunk_pos) {
    if (isChunkLoaded(_chunk_pos)) {
        return m_chunks.at(_chunk_pos);
    }
    return nullptr;
}

bool World::addChunk(const glm::ivec3 &_chunk_pos) {
    if (isChunkLoaded(_chunk_pos))
        return false;

    m_chunks.insert({_chunk_pos, new Chunk(_chunk_pos, Chunk::GenType::SUPERFLAT)});
    m_chunks_frontier.erase(_chunk_pos);
    Chunk *inserted_chunk = m_chunks.at(_chunk_pos);

    // on ajoute tous ses voisins dans la frontière si ils ne sont pas déjà chargé sinon on update le chunk car il a un nouveau voisin !
    for (int face_i = 0; face_i < 6; face_i++) {
        glm::ivec3 neighbour_pos = _chunk_pos + Chunk::NEIGHBOURS_POS[face_i];
        Chunk *neighbour = findChunk(neighbour_pos);
        if (neighbour == nullptr) {
            m_chunks_frontier.insert(neighbour_pos);
        } else {
            inserted_chunk->m_neighbours[face_i] = neighbour;
            inserted_chunk->updateBlockNeighbours(face_i);

            neighbour->m_neighbours[OPPOSITE_FACE[face_i]] = inserted_chunk;
            neighbour->buildMesh();
        }
    }

    inserted_chunk->buildMesh();

    return true;
}
bool World::removeChunk(const glm::ivec3 &_chunk_pos) {
    Chunk *removed_chunk = findChunk(_chunk_pos);
    if (removed_chunk == nullptr)
        return false;
    delete removed_chunk;
    m_chunks.erase(_chunk_pos);
    m_chunks_frontier.erase(_chunk_pos);

    for (int face_i = 0; face_i < 6; face_i++) {
        glm::ivec3 neighbour_pos = _chunk_pos + Chunk::NEIGHBOURS_POS[face_i];
        Chunk *neighbour = findChunk(neighbour_pos);
        if (neighbour != nullptr) {
            // neighbour chunk is loaded, we update it and put the current chunk in the frontier
            m_chunks_frontier.insert(_chunk_pos);
            neighbour->m_neighbours[OPPOSITE_FACE[face_i]] = nullptr;
            neighbour->updateBlockNeighbours(OPPOSITE_FACE[face_i]);
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

bool World::generate(const glm::ivec3 &_chunk_pos) {
    if (!isChunkLoaded(_chunk_pos)) {
        addChunk(_chunk_pos);
        return true;
    }

    std::list<glm::ivec3> chunk_to_remove;
    for (auto &[chunk_pos, chunk] : m_chunks) {
        if (Chunk::chunkDistance(chunk_pos, _chunk_pos) > RENDER_DISTANCE) {
            chunk_to_remove.push_back(chunk_pos);
        }
    }
    for (const glm::ivec3 &chunk_pos : chunk_to_remove) {
        removeChunk(chunk_pos);
    }
    if (!chunk_to_remove.empty()) {
        return true;
    }

    std::map<uint, glm::ivec3> chunk_to_add;
    for (const glm::ivec3 &chunk_pos : m_chunks_frontier) {
        uint chunk_dist = Chunk::chunkDistance(chunk_pos, _chunk_pos);
        if (chunk_dist <= RENDER_DISTANCE) {
            chunk_to_add.insert({chunk_dist, chunk_pos});
        }
    }
    if (!chunk_to_add.empty()) {
        addChunk(chunk_to_add.begin()->second);
        return true;
    }

    return false;
}
