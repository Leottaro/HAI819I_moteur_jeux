// USUAL INCLUDES
#include "World.hpp"

#include <list>

Chunk* World::findChunk(const glm::ivec3& _chunk_pos) {
    return isChunkLoaded(_chunk_pos) ? m_chunks.at(_chunk_pos)
                                     : nullptr;
}

Block* World::findBlock(const glm::ivec3& _block_pos) {
    glm::ivec3 chunk_pos = Chunk::blockPosToChunkPos(_block_pos);
    return isChunkLoaded(chunk_pos) ? &m_chunks.at(chunk_pos)->getBlock(_block_pos)
                                    : nullptr;
}

std::vector<const Block*> World::findSolidBlocks(const glm::ivec3& start, const glm::ivec3& end) {
    Chunk* start_chunk = findChunk(Chunk::blockPosToChunkPos(start));
    std::vector<const Block*> res;
    if (start_chunk != nullptr)
        start_chunk->findSolidBlocks(start, end, res);
    return res;
}

Chunk* World::addChunk(const glm::ivec3& _chunk_pos) {
    if (isChunkLoaded(_chunk_pos))
        return nullptr;

    m_chunks.add(Chunk(this, _chunk_pos, m_gentype));
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
            neighbour->should_rebuild_mesh = true;
        }
    }

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
            neighbour->should_rebuild_mesh = true;
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

bool World::generate(const glm::vec3& _pos) {
    glm::ivec3 _chunk_pos = Chunk::posToChunkPos(_pos);
    if (!isChunkLoaded(_chunk_pos)) {
        addChunk(_chunk_pos);
        std::cout << m_chunks.size() << " CHUNKS" << std::endl;
        return true;
    }

    std::list<glm::ivec3> chunk_to_remove;
    m_chunks.forEach([&](Chunk& chunk) {
        if (Chunk::chunkDistance(_pos, chunk.getPos()) > m_render_distance) {
            chunk_to_remove.push_back(chunk.getPos());
        }
    });
    for (const glm::ivec3& chunk_pos : chunk_to_remove) {
        removeChunk(chunk_pos);
    }
    if (!chunk_to_remove.empty()) {
        std::cout << m_chunks.size() << " CHUNKS" << std::endl;
        return true;
    }

    std::map<float, glm::ivec3> chunk_to_add;
    for (const glm::ivec3& chunk_pos : m_chunks_frontier) {
        float chunk_dist = Chunk::chunkDistance(_pos, chunk_pos);
        if (chunk_dist <= m_render_distance) {
            chunk_to_add.insert({chunk_dist, chunk_pos});
        }
    }
    if (!chunk_to_add.empty()) {
        addChunk(chunk_to_add.begin()->second);
        std::cout << m_chunks.size() << " CHUNKS" << std::endl;
        return true;
    }

    std::cout << m_chunks.size() << " CHUNKS" << std::endl;
    return false;
}

ECS::EntityId World::addTestEntity(const glm::vec3& _pos) {
    return m_ecs_manager.createEntity<ECS::TestEntity>({ECS::Positionnable{this, _pos}});
}