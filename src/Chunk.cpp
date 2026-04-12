// USUAL INCLUDES
#include "Chunk.hpp"
#include <stdexcept>

void Chunk::updateBlockNeighbours(uint8_t _face_i) {
    uint8_t face_axis = _face_i % 3;
    uint8_t face_depth = _face_i / 3;
    int block_i = 0;
    int neighbour_i = face_axis == 0   ? CHUNK_SIZE * (CHUNK_SIZE - 1)
                      : face_axis == 1 ? CHUNK_SIZE - 1
                                       : CHUNK_SIZE * CHUNK_SIZE * (CHUNK_SIZE - 1);
    int i_step = face_axis == 0   ? 1
                 : face_axis == 1 ? CHUNK_SIZE
                                  : 1;
    int j_step = face_axis == 0   ? CHUNK_SIZE * (CHUNK_SIZE - 1)
                 : face_axis == 1 ? 0
                                  : 0;
    if (face_depth != 0) {
        std::swap(block_i, neighbour_i);
    }

    bool has_neighbour = m_neighbours[_face_i] == nullptr;
    for (size_t j = 0; j < CHUNK_SIZE; j++) {
        for (size_t i = 0; i < CHUNK_SIZE; i++) {
            Block *block = &m_blocks[block_i];
            if (has_neighbour) {
                block->m_neighbours[_face_i] = nullptr;
            } else {
                Block *neighbour_block = &m_neighbours[_face_i]->m_blocks[neighbour_i];
                block->m_neighbours[_face_i] = neighbour_block;
                neighbour_block->m_neighbours[OPPOSITE_FACE[_face_i]] = block;
            }
            block_i += i_step;
            neighbour_i += i_step;
        }
        block_i += j_step;
        neighbour_i += j_step;
    }
}

void Chunk::initNeighbours() {
    glm::uvec3 pos;
    glm::ivec3 world_pos;
    world_pos.y = m_pos.y;
    for (pos.y = 0; pos.y < CHUNK_SIZE; pos.y++) {
        world_pos.z = m_pos.z;
        for (pos.z = 0; pos.z < CHUNK_SIZE; pos.z++) {
            world_pos.x = m_pos.x;
            for (pos.x = 0; pos.x < CHUNK_SIZE; pos.x++) {
                Block &block = m_blocks[posToBlockI(pos)];
                for (uint8_t _face_i = 0; _face_i < 6; _face_i++) {
                    glm::ivec3 neighbour_pos = glm::ivec3(pos) + Block::NEIGHBOURS_POS[_face_i];
                    if (neighbour_pos.x < 0 || neighbour_pos.y < 0 || neighbour_pos.z < 0 || neighbour_pos.x >= CHUNK_SIZE || neighbour_pos.y >= CHUNK_SIZE || neighbour_pos.z >= CHUNK_SIZE) {
                        block.m_neighbours[_face_i] = m_neighbours[_face_i] == nullptr
                                                          ? nullptr
                                                          : &m_neighbours[_face_i]->m_blocks[posToBlockI((neighbour_pos.x + CHUNK_SIZE) % CHUNK_SIZE, (neighbour_pos.y + CHUNK_SIZE) % CHUNK_SIZE, (neighbour_pos.z + CHUNK_SIZE) % CHUNK_SIZE)];
                    } else {
                        block.m_neighbours[_face_i] = &m_blocks[posToBlockI(neighbour_pos)];
                    }
                }
                world_pos.x++;
            }
            world_pos.z++;
        }
        world_pos.y++;
    }
}

void Chunk::generate(GenType _type) {
    glm::uvec3 pos;
    glm::ivec3 world_pos;
    switch (_type) {
    case GenType::SUPERFLAT:
        world_pos.y = m_pos.y;
        for (pos.y = 0; pos.y < CHUNK_SIZE; pos.y++) {
            world_pos.z = m_pos.z;
            for (pos.z = 0; pos.z < CHUNK_SIZE; pos.z++) {
                world_pos.x = m_pos.x;
                for (pos.x = 0; pos.x < CHUNK_SIZE; pos.x++) {
                    Block &block = m_blocks[posToBlockI(pos)];
                    block.getPos() = world_pos;
                    if (world_pos.y <= -45) {
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

                    world_pos.x++;
                }
                world_pos.z++;
            }
            world_pos.y++;
        }
        break;
    default:
        throw std::runtime_error("ChunkGenType not supported in Chunk generation");
    }
}

Chunk::Chunk(const glm::ivec3 &_chunk_pos, GenType _type) : m_pos(_chunk_pos), m_aabb(glm::vec3(m_pos), glm::vec3(m_pos) + glm::vec3(CHUNK_SIZE)) {
    initNeighbours();
    generate(_type);
}

void Chunk::buildMesh() {
    m_mesh.clear();
    std::vector<glm::vec3> &positions = m_mesh.vertexPositions();
    std::vector<glm::vec3> &normals = m_mesh.vertexNormals();
    std::vector<glm::vec2> &uvs = m_mesh.vertexTexCoords();
    std::vector<glm::uvec3> &triangles = m_mesh.triangleIndices();

    glm::uvec3 pos(0);
    glm::ivec3 world_pos;
    world_pos.y = m_pos.y;
    for (pos.y = 0; pos.y < CHUNK_SIZE; pos.y++) {
        world_pos.z = m_pos.z;
        for (pos.z = 0; pos.z < CHUNK_SIZE; pos.z++) {
            world_pos.x = m_pos.x;
            for (pos.x = 0; pos.x < CHUNK_SIZE; pos.x++) {
                Block &block = m_blocks[posToBlockI(pos)];
                if (block.getType() == Block::BlockType::Air) {
                    world_pos.x++;
                    continue;
                }

                for (int face_i = 0; face_i < 6; face_i++) {
                    const Block *neighbour = block.m_neighbours[face_i];
                    if (neighbour != nullptr && neighbour->getType() != Block::BlockType::Air)
                        continue;
                    const Block::FaceData &face = Block::FACE_DATA[face_i];

                    for (int i = 0; i < 4; ++i) {
                        positions.push_back(glm::vec3(world_pos) + face.vertices[i]);
                        normals.push_back(face.normal);
                    }
                    glm::uvec3 offset(positions.size() - 4);
                    triangles.push_back(face.triangles[0] + offset);
                    triangles.push_back(face.triangles[1] + offset);
                }
                world_pos.x++;
            }
            world_pos.z++;
        }
        world_pos.y++;
    }

    if (m_mesh.nbVertices() > 0) {
        uvs.resize(positions.size());
        m_mesh.initShaderData();
    }
    m_aabb.clearShaderData();
    m_aabb.initShaderData();
}
