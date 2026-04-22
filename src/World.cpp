// USUAL INCLUDES
#include "World.hpp"
#include <list>

namespace uuid {
static std::random_device rd;
static std::mt19937 gen(rd());
static std::uniform_int_distribution<> dis(0, 15);
static std::uniform_int_distribution<> dis2(8, 11);

std::string generate_uuid_v4() {
    std::stringstream ss;
    int i;
    ss << std::hex;
    for (i = 0; i < 8; i++) {
        ss << dis(gen);
    }
    ss << "-";
    for (i = 0; i < 4; i++) {
        ss << dis(gen);
    }
    ss << "-4";
    for (i = 0; i < 3; i++) {
        ss << dis(gen);
    }
    ss << "-";
    ss << dis2(gen);
    for (i = 0; i < 3; i++) {
        ss << dis(gen);
    }
    ss << "-";
    for (i = 0; i < 12; i++) {
        ss << dis(gen);
    };
    return ss.str();
}
} // namespace uuid

Chunk *World::findChunk(const glm::ivec3 &_chunk_pos) {
    if (isChunkLoaded(_chunk_pos)) {
        return m_chunks.at(_chunk_pos);
    }
    return nullptr;
}

Block *World::findBlock(const glm::ivec3 &_block_pos) {
    glm::ivec3 chunk_pos = Chunk::posToChunkPos(_block_pos);
    if (isChunkLoaded(chunk_pos)) {
        return &m_chunks.at(chunk_pos)->getBlock(_block_pos);
    }
    return nullptr;
}

Chunk *World::addChunk(const glm::ivec3 &_chunk_pos) {
    if (isChunkLoaded(_chunk_pos))
        return nullptr;

    m_chunks.insert({_chunk_pos, new Chunk(this, _chunk_pos, Chunk::GenType::SUPERFLAT)});
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
            neighbour->m_neighbours[OPPOSITE_FACE[face_i]] = inserted_chunk;

            inserted_chunk->updateBlockNeighbours(face_i);
            neighbour->updateShaderData();
        }
    }

    inserted_chunk->updateShaderData();

    return inserted_chunk;
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
            neighbour->updateShaderData();
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

bool World::generate(const glm::vec3 &_pos) {
    glm::ivec3 _chunk_pos = Chunk::posToChunkPos(_pos);
    if (!isChunkLoaded(_chunk_pos)) {
        addChunk(_chunk_pos);
        return true;
    }

    std::list<glm::ivec3> chunk_to_remove;
    for (auto &[chunk_pos, chunk] : m_chunks) {
        if (Chunk::chunkDistance(_pos, chunk_pos) > RENDER_DISTANCE) {
            chunk_to_remove.push_back(chunk_pos);
        }
    }
    for (const glm::ivec3 &chunk_pos : chunk_to_remove) {
        removeChunk(chunk_pos);
    }
    if (!chunk_to_remove.empty()) {
        return true;
    }

    std::map<float, glm::ivec3> chunk_to_add;
    for (const glm::ivec3 &chunk_pos : m_chunks_frontier) {
        float chunk_dist = Chunk::chunkDistance(_pos, chunk_pos);
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

Entity *World::findEntity(const std::string &_uuid) {
    if (isEntityLoaded(_uuid)) {
        return m_entities.at(_uuid);
    }
    return nullptr;
}

Entity *World::addEntity(Entity::Type _type, const glm::vec3 &_pos) {
    glm::ivec3 chunk_pos = Chunk::posToChunkPos(_pos);
    Chunk *chunk = addChunk(chunk_pos);
    if (chunk == nullptr)
        return nullptr;

    std::string uuid = uuid::generate_uuid_v4();
    m_entities.insert({uuid, new Entity(_type, uuid, chunk, _pos)});
    return m_entities.at(uuid);
}

bool World::removeEntity(const std::string &_uuid) {
    Entity *removed_entity = findEntity(_uuid);
    if (removed_entity == nullptr)
        return false;
    delete removed_entity;
    m_entities.erase(_uuid);
    return true;
}

void World::update(float _deltaTime) {
    std::vector<std::string> entities_to_destroy;
    entities_to_destroy.reserve(m_entities.size());
    for (auto &[uuid, entity] : m_entities) {
        if (!entity->update(_deltaTime)) {
            entities_to_destroy.push_back(uuid);
        }
    }

    for (const std::string &uuid : entities_to_destroy) {
        removeEntity(uuid);
    }
}