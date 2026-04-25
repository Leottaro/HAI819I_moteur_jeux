// USUAL INCLUDES
#include "Chunk.hpp"
#include "World.hpp"
#include <stdexcept>
#include <iostream>

Chunk *Chunk::getChunk(const glm::vec3 &_pos) {
    return m_world->findChunk(Chunk::posToChunkPos(_pos));
}

// https://stackoverflow.com/questions/55263298/draw-all-voxels-that-pass-through-a-3d-line-in-3d-voxel-space
void Chunk::findSolidBlocks(glm::ivec3 start, const glm::ivec3 &_end, std::vector<Block *> &blocks) {
    glm::vec3 delta = glm::abs(_end - start);
    glm::ivec3 step(start.x < _end.x ? 1 : -1, start.y < _end.y ? 1 : -1, start.z < _end.z ? 1 : -1);
    float d_length = glm::length(delta);
    glm::vec3 tdelta = d_length / delta;
    glm::vec3 tmax = tdelta * 0.5f;

    Block *block = getBlock(start);
    glm::ivec3 start_chunk = blockPosToChunkPos(start);
    if (m_pos != start_chunk) {
        Chunk *chunk = m_world->findChunk(start_chunk);
        if (chunk != nullptr) {
            chunk->findSolidBlocks(start, _end, blocks);
            return;
        }
    }

    while (start != _end) {
        if (block->hasHitbox()) {
            blocks.push_back(block);
        }

        if (tmax.x < tmax.y) {
            if (tmax.x < tmax.z) {
                start.x += step.x;
                tmax.x += tdelta.x;
            } else if (tmax.x > tmax.z) {
                start.z += step.z;
                tmax.z += tdelta.z;
            } else {
                start.x += step.x;
                start.z += step.z;
                tmax.x += tdelta.x;
                tmax.z += tdelta.z;
            }
        } else if (tmax.x > tmax.y) {
            if (tmax.y < tmax.z) {
                start.y += step.y;
                tmax.y += tdelta.y;
            } else if (tmax.y > tmax.z) {
                start.z += step.z;
                tmax.z += tdelta.z;
            } else {
                start.y += step.y;
                start.z += step.z;
                tmax.y += tdelta.y;
                tmax.z += tdelta.z;
            }
        } else {
            if (tmax.y < tmax.z) {
                start.y += step.y;
                start.x += step.x;
                tmax.y += tdelta.y;
                tmax.x += tdelta.x;
            } else if (tmax.y > tmax.z) {
                start.z += step.z;
                tmax.z += tdelta.z;
            } else {
                start += step;
            }
        }

        start_chunk = blockPosToChunkPos(start);
        if (m_pos != start_chunk) {
            Chunk *chunk = m_world->findChunk(start_chunk);
            if (chunk != nullptr) {
                chunk->findSolidBlocks(start, _end, blocks);
                return;
            }
        }

        block = getBlock(start);
    }

    if (block->hasHitbox()) {
        blocks.push_back(block);
    }
}

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
    glm::ivec3 world_pos;
    int block_i = -1;
    std::array<bool, 3> neighbour_exists{false};
    for (world_pos.y = m_pos.y; world_pos.y < m_pos.y + CHUNK_SIZE; world_pos.y++) {
        neighbour_exists[2] = world_pos.y > m_pos.y;
        for (world_pos.z = m_pos.z; world_pos.z < m_pos.z + CHUNK_SIZE; world_pos.z++) {
            neighbour_exists[0] = world_pos.z > m_pos.z;
            for (world_pos.x = m_pos.x; world_pos.x < m_pos.x + CHUNK_SIZE; world_pos.x++) {
                neighbour_exists[1] = world_pos.x > m_pos.x;
                block_i++;
                for (uint8_t _face_i = 0; _face_i < 3; _face_i++) {
                    if (neighbour_exists[_face_i]) {
                        int neighbour_i = block_i + BLOCK_NEIGHBOUR_I_OFFSET[_face_i];
                        m_blocks[block_i].m_neighbours[_face_i] = &m_blocks[neighbour_i];
                        m_blocks[neighbour_i].m_neighbours[OPPOSITE_FACE[_face_i]] = &m_blocks[block_i];
                    }
                }
            }
        }
    }
}

void Chunk::generate(GenType _type) {
    glm::ivec3 world_pos;
    size_t block_i = 0;
    switch (_type) {
    case GenType::SUPERFLAT:
        for (world_pos.y = m_pos.y; world_pos.y < m_pos.y + CHUNK_SIZE; world_pos.y++) {
            for (world_pos.z = m_pos.z; world_pos.z < m_pos.z + CHUNK_SIZE; world_pos.z++) {
                for (world_pos.x = m_pos.x; world_pos.x < m_pos.x + CHUNK_SIZE; world_pos.x++) {
                    Block &block = m_blocks[block_i++];
                    block.getPos() = world_pos;

                    if (world_pos.y <= -45) {
                        block.getType() = Block::Type::Air;
                    } else if (world_pos.y <= 0) {
                        block.getType() = Block::Type::Stone;
                    } else if (world_pos.y <= 3) {
                        block.getType() = Block::Type::Dirt;
                    } else if (world_pos.y <= 16) {
                        // uint truc = (world_pos.y * 43 + world_pos.z) * 37 + world_pos.x;
                        // block.getType() = Block::Type(truc % Block::BLOCK_TYPES_N);
                        block.getType() = (world_pos.x % 2) == (world_pos.z % 2) ? Block::Type::Air : Block::Type::Grass;
                    } else {
                        block.getType() = Block::Type::Air;
                    }

                    if (world_pos.x % CHUNK_SIZE == 0 || world_pos.y % CHUNK_SIZE == 0 || world_pos.z % CHUNK_SIZE == 0)
                        block.getType() = Block::Type::Glass;

                    // block.getType() = world_pos.x % 2 == world_pos.y % 2 && world_pos.y % 2 == world_pos.z % 2 ? Block::Type::Stone : Block::Type::Air;
                }
            }
        }
        break;
    default:
        throw std::runtime_error("ChunkGenType not supported in Chunk generation");
    }
}

Chunk::Chunk(World *_world, const glm::ivec3 &_chunk_pos, GenType _type) : m_world(_world), m_pos(_chunk_pos), m_aabb(glm::vec3(m_pos), glm::vec3(m_pos) + glm::vec3(CHUNK_SIZE)) {
    initNeighbours();
    generate(_type);
    initShaderData();
    updateShaderData();
}

struct ChunkVertex {
    glm::u8vec3 position;
    glm::i8vec3 normal;
    uint8_t _pad[2]; // explicit 2-byte pad
    glm::vec2 uv;
};
// Si mes comptes sont bons on a 28.5MiB pour tout les chunks (c'est OK)
namespace ChunkMeshScratch {
std::array<ChunkVertex, Chunk::MAX_VERTICES> vertices;
std::array<glm::uvec3, Chunk::MAX_TRIANGLES> triangles;
}; // namespace ChunkMeshScratch
void Chunk::initShaderData() {
    glGenVertexArrays(1, &m_VAO);
    glBindVertexArray(m_VAO);

    glGenBuffers(1, &m_VBO);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    // Attribute 0: position
    glEnableVertexAttribArray(0);
    glVertexAttribIPointer(0, 3, GL_UNSIGNED_BYTE, sizeof(ChunkVertex), (void *)offsetof(ChunkVertex, position));
    // Attribute 1: normal
    glEnableVertexAttribArray(1);
    glVertexAttribIPointer(1, 3, GL_BYTE, sizeof(ChunkVertex), (void *)offsetof(ChunkVertex, normal));
    // Attribute 0: uv
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(ChunkVertex), (void *)offsetof(ChunkVertex, uv));

    glGenBuffers(1, &m_EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);

    m_aabb.initShaderData();
}

void Chunk::updateShaderData() {
    m_vertices_count = m_triangles_count = 0;

    glm::u8vec3 local_pos;
    int block_i = -1;
    for (local_pos.y = 0; local_pos.y < CHUNK_SIZE; local_pos.y++) {
        for (local_pos.z = 0; local_pos.z < CHUNK_SIZE; local_pos.z++) {
            for (local_pos.x = 0; local_pos.x < CHUNK_SIZE; local_pos.x++) {
                block_i++;
                Block &block = m_blocks[block_i];
                if (block.getType() == Block::Type::Air) {
                    continue;
                }

                for (int face_i = 0; face_i < 6; face_i++) {
                    const Block *neighbour = block.m_neighbours[face_i];
                    if (neighbour != nullptr && (!neighbour->isTransparent() || block.getType() == neighbour->getType()))
                        continue;
                    const Block::FaceData &face = Block::FACE_DATA[face_i];

                    std::array<glm::vec2, 4> face_uvs = Block::getUV(block.getType(), face_i);
                    for (int i = 0; i < 4; ++i) {
                        ChunkMeshScratch::vertices[m_vertices_count].position = local_pos + face.vertices[i];
                        ChunkMeshScratch::vertices[m_vertices_count].normal = face.normal;
                        ChunkMeshScratch::vertices[m_vertices_count].uv = face_uvs[i];
                        m_vertices_count++;
                    }

                    glm::uvec3 offset(m_vertices_count - 4);
                    ChunkMeshScratch::triangles[m_triangles_count++] = face.triangles[0] + offset;
                    ChunkMeshScratch::triangles[m_triangles_count++] = face.triangles[1] + offset;
                }
            }
        }
    }

    glBindVertexArray(m_VAO);

    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(m_vertices_count * sizeof(ChunkVertex)), ChunkMeshScratch::vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(m_triangles_count * sizeof(glm::uvec3)), ChunkMeshScratch::triangles.data(), GL_STATIC_DRAW);
}

void Chunk::render() {
    glBindVertexArray(m_VAO);
    glDrawElements(GL_TRIANGLES, m_triangles_count * 3, GL_UNSIGNED_INT, 0);
}

void Chunk::renderDebugBox() {
    m_aabb.render();
}
void Chunk::clearShaderData() {
    if (m_VAO) {
        glDeleteVertexArrays(1, &m_VAO);
        m_VAO = 0;
    }
    if (m_VBO) {
        glDeleteBuffers(1, &m_VBO);
        m_VBO = 0;
    }
    if (m_EBO) {
        glDeleteBuffers(1, &m_EBO);
        m_EBO = 0;
    }
    m_aabb.clearShaderData();
}
