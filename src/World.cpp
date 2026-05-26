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

std::vector<const Block*> World::findBlockLine(const glm::vec3& start, const glm::vec3& end) const {
    const Chunk* start_chunk = findChunk(Chunk::posToChunkPos(start));
    return start_chunk != nullptr ? start_chunk->findBlockLine(start, end) : std::vector<const Block*>{};
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
                glm::ivec3 neighbour_neighbour_pos = neighbour_pos + Chunk::NEIGHBOURS_POS[neighbour_face_i];
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

void World::generate(const std::vector<glm::vec3>& _centers) {
    std::vector<glm::ivec3> chunk_centers;
    for (const glm::vec3& pos : _centers)
        chunk_centers.push_back(Chunk::blockPosToChunkPos(pos));
    uint squared_render_distance = m_render_distance * m_render_distance;

    for (const glm::ivec3& chunk_pos : chunk_centers)
        if (findChunk(chunk_pos) == nullptr)
            addChunk(chunk_pos);

    std::optional<glm::ivec3> chunk_to_remove;
    m_chunks.forEach([&chunk_to_remove, &chunk_centers, squared_render_distance](Chunk& chunk) {
        if (chunk_to_remove.has_value())
            return;
        bool should_remove = true;
        for (const glm::ivec3& chunk_pos : chunk_centers) {
            if (Chunk::chunkSquaredDistance(chunk.getPos(), chunk_pos) <= squared_render_distance) {
                should_remove = false;
                break;
            }
        }
        if (should_remove)
            chunk_to_remove = chunk.getPos();
    });
    if (chunk_to_remove.has_value()) {
        removeChunk(chunk_to_remove.value());
    }

    std::vector<std::pair<float, glm::ivec3>> chunk_to_add(_centers.size(), {std::numeric_limits<uint>::max(), glm::ivec3(0)});
    for (const glm::ivec3& chunk_pos : m_chunks_frontier) {
        for (uint i = 0; i < _centers.size(); i++) {
            uint squared_dist = Chunk::chunkSquaredDistance(chunk_centers[i], chunk_pos);
            if (squared_dist > squared_render_distance)
                continue;

            float real_squared_dist = glm::distance2(_centers[i], glm::vec3(chunk_pos) + glm::vec3(Chunk::CHUNK_SIZE / 2));
            if (real_squared_dist < chunk_to_add[i].first) {
                chunk_to_add[i].first = real_squared_dist;
                chunk_to_add[i].second = chunk_pos;
            }
        }
    }

    for (uint i = 0; i < _centers.size(); i++)
        if (chunk_to_add[i].first < std::numeric_limits<float>::max())
            addChunk(chunk_to_add[i].second);
}

void World::selfUpdate(const std::vector<glm::vec3>& _centers) {
    if (!m_play)
        return;
    m_world_time++;

    // load / unload the chunks
    generate(_centers);

    // update the chunks (water, redstone, ....)
}