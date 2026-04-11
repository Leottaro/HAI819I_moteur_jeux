// USUAL INCLUDES
#include "Chunk.hpp"
#include <stdexcept>

void Chunk::recomputeBlockNeighbours() {
    foreachBlock([this](const glm::uvec3 &pos, const glm::ivec3 &world_pos, Block &block) {
        for (int face_i = 0; face_i < 6; face_i++) {
            glm::ivec3 neighbour_pos = glm::ivec3(pos) + Block::NEIGHBOURS_POS[face_i];
            if (neighbour_pos.x < 0 || neighbour_pos.y < 0 || neighbour_pos.z < 0 || neighbour_pos.x >= CHUNK_SIZE || neighbour_pos.y >= CHUNK_SIZE || neighbour_pos.z >= CHUNK_SIZE) {
                block.m_neighbours[face_i] = m_neighbours[face_i] == nullptr
                                                 ? nullptr
                                                 : &m_neighbours[face_i]->m_blocks[posToBlockI((neighbour_pos.x + CHUNK_SIZE) % CHUNK_SIZE, (neighbour_pos.y + CHUNK_SIZE) % CHUNK_SIZE, (neighbour_pos.z + CHUNK_SIZE) % CHUNK_SIZE)];

                continue;
            }
            block.m_neighbours[face_i] = &m_blocks[posToBlockI(neighbour_pos)];
        }
    });
}

Chunk::~Chunk() {
    clear();
}

Chunk::Chunk(const glm::ivec3 &_chunk_pos, GenType _type) : m_pos(_chunk_pos), m_aabb(glm::vec3(m_pos), glm::vec3(m_pos) + glm::vec3(CHUNK_SIZE)) {
    switch (_type) {
    case GenType::SUPERFLAT:
        foreachBlock([](const glm::uvec3 &pos, const glm::ivec3 &world_pos, Block &block) {
            block.getPos() = world_pos;
            if (world_pos.y <= -1) {
                block.getType() = Block::BlockType::Air;
            } else if (world_pos.y <= 0) {
                block.getType() = Block::BlockType::Bedrock;
            } else if (world_pos.y <= 3) {
                block.getType() = Block::BlockType::Dirt;
            } else if (world_pos.y <= 4) {
                block.getType() = Block::BlockType::Grass;
            } else {
                block.getType() = Block::BlockType::Air;
            }

            if (pos.x == 0 && pos.z == 0) {
                block.getType() = pos.y % 2 == 0 ? Block::BlockType::Air : Block::BlockType::Dirt;
            }
        });
        break;
    default:
        throw std::runtime_error("ChunkGenType not supported in Chunk generation");
    }
}

void Chunk::buildMesh() {
    m_mesh.clear();
    std::vector<glm::vec3> &positions = m_mesh.vertexPositions();
    std::vector<glm::vec3> &normals = m_mesh.vertexNormals();
    std::vector<glm::vec2> &uvs = m_mesh.vertexTexCoords();
    std::vector<glm::uvec3> &triangles = m_mesh.triangleIndices();

    foreachBlock([&positions, &normals, &triangles, &uvs](const glm::uvec3 &pos, const glm::ivec3 &world_pos, Block &block) {
        if (block.getType() == Block::BlockType::Air)
            return;

        for (int face_i = 0; face_i < 6; face_i++) {
            if (block.m_neighbours[face_i] != nullptr && block.m_neighbours[face_i]->getType() != Block::BlockType::Air) {
                continue;
            }

            const Block::FaceData &face = Block::FACE_DATA[face_i];

            for (int i = 0; i < 4; ++i) {
                positions.push_back(glm::vec3(world_pos) + face.vertices[i]);
                normals.push_back(face.normal);
            }
            glm::uvec3 offset(positions.size() - 4);
            triangles.push_back(face.triangles[0] + offset);
            triangles.push_back(face.triangles[1] + offset);
        }
    });
    uvs.resize(positions.size());
    m_mesh.initShaderData();
    m_aabb.initShaderData();
}
