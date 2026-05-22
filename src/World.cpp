// USUAL INCLUDES
#include "World.hpp"

Block* World::findBlock(const glm::ivec3& _block_pos) {
    glm::ivec3 chunk_pos = Chunk::blockPosToChunkPos(_block_pos);
    Chunk* chunk = m_chunks.at(chunk_pos);
    return chunk != nullptr ? &chunk->getBlock(_block_pos)
                            : nullptr;
}
const Block* World::findBlock(const glm::ivec3& _block_pos) const {
    glm::ivec3 chunk_pos = Chunk::blockPosToChunkPos(_block_pos);
    const Chunk* chunk = m_chunks.at(chunk_pos);
    return chunk != nullptr ? &chunk->getBlock(_block_pos)
                            : nullptr;
}

std::vector<const Block*> World::findSolidBlocks(const glm::ivec3& start, const glm::ivec3& end) const {
    const Chunk* start_chunk = findChunk(Chunk::blockPosToChunkPos(start));
    std::vector<const Block*> res;
    if (start_chunk != nullptr)
        start_chunk->findSolidBlocks(start, end, res);
    return res;
}

Chunk* World::addChunk(const glm::ivec3& _chunk_pos) {
    if (findChunk(_chunk_pos) != nullptr)
        return nullptr;

    m_chunks.emplace(_chunk_pos, this, _chunk_pos, m_gentype);
    m_chunks_frontier.erase(_chunk_pos);
    Chunk* inserted_chunk = m_chunks.at(_chunk_pos);

    // on ajoute tous ses voisins dans la frontière si ils ne sont pas déjà chargé sinon on update le chunk car il a un nouveau voisin !
    for (int face_i = 0; face_i < 6; face_i++) {
        glm::ivec3 neighbour_pos = _chunk_pos + Chunk::NEIGHBOURS_POS[face_i];
        Chunk* neighbour = findChunk(neighbour_pos);
        if (neighbour == nullptr) {
            m_chunks_frontier.insert(neighbour_pos);
        } else {
            inserted_chunk->m_neighbours[face_i] = neighbour;
            neighbour->m_neighbours[OPPOSITE_FACE[face_i]] = inserted_chunk;

            inserted_chunk->updateBlockNeighbours(face_i);
            if (neighbour->m_should_rebuild_mesh != nullptr)
                *neighbour->m_should_rebuild_mesh = true;
        }
    }

    m_added_chunks.push_back(inserted_chunk);
    return inserted_chunk;
}
bool World::removeChunk(const glm::ivec3& _chunk_pos) {
    Chunk* removed_chunk = findChunk(_chunk_pos);
    if (removed_chunk == nullptr)
        return false;
    m_chunks.remove(_chunk_pos);
    m_chunks_frontier.erase(_chunk_pos);

    for (int face_i = 0; face_i < 6; face_i++) {
        glm::ivec3 neighbour_pos = _chunk_pos + Chunk::NEIGHBOURS_POS[face_i];
        Chunk* neighbour = findChunk(neighbour_pos);
        if (neighbour != nullptr) {
            // neighbour chunk is loaded, we update it and put the current chunk in the frontier
            m_chunks_frontier.insert(_chunk_pos);
            neighbour->m_neighbours[OPPOSITE_FACE[face_i]] = nullptr;
            neighbour->updateBlockNeighbours(OPPOSITE_FACE[face_i]);
            if (neighbour->m_should_rebuild_mesh != nullptr)
                *neighbour->m_should_rebuild_mesh = true;
        } else if (isChunkFrontier(neighbour_pos)) {
            // neighbour chunk is in frontier, remove it if it has no loaded neighbour.
            bool neighbour_has_neighbour = false;
            for (int neighbour_face_i = 0; neighbour_face_i < 6; neighbour_face_i++) {
                glm::ivec3 neighbour_neighbour_pos = neighbour_pos + Chunk::NEIGHBOURS_POS[face_i];
                if (findChunk(neighbour_neighbour_pos) != nullptr) {
                    neighbour_has_neighbour = true;
                    break;
                }
            }
            if (!neighbour_has_neighbour) {
                m_chunks_frontier.erase(neighbour_pos);
            }
        }
    }

    m_removed_chunks.push_back(_chunk_pos);
    return true;
}

bool World::generate(const std::vector<glm::vec3>& _centers) {
    bool chunk_in_pos = false;
    for (const glm::vec3& pos : _centers) {
        glm::ivec3 _chunk_pos = Chunk::posToChunkPos(pos);
        if (findChunk(_chunk_pos) == nullptr) {
            addChunk(_chunk_pos);
            chunk_in_pos = true;
        }
    }
    if (chunk_in_pos)
        return true;

    bool removed = false;
    m_chunks.forEach([&](Chunk& chunk) {
        bool should_remove = true;
        for (const glm::vec3& pos : _centers) {
            if (Chunk::chunkDistance(chunk.getPos(), pos) <= m_render_distance) {
                should_remove = false;
                break;
            }
        }
        if (should_remove) {
            removeChunk(chunk.getPos());
            removed = true;
        }
    });
    if (removed)
        return true;

    std::vector<std::map<float, glm::ivec3>> chunk_to_add(_centers.size());
    for (const glm::ivec3& chunk_pos : m_chunks_frontier) {
        for (uint i = 0; i < _centers.size(); i++) {
            float dist = Chunk::chunkDistance(_centers[i], chunk_pos);
            chunk_to_add[i].insert({dist, chunk_pos});
        }
    }

    bool res = false;
    for (uint i = 0; i < _centers.size(); i++) {
        if (!chunk_to_add[i].empty()) {
            addChunk(chunk_to_add[i].begin()->second);
            res = true;
        }
    }

    return res;
}

void World::update(std::vector<glm::vec3> _centers) {
    if (!m_play)
        return;
    m_world_time++;

    // load / unload the chunks
    generate(_centers);

    // update the chunks (water, redstone, ....)
}