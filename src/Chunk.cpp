// USUAL INCLUDES
#include "Chunk.hpp"
#include "World.hpp"
#include "objects/blocks.hpp"
#include "objects/textures.hpp"

#include <stdexcept>

Chunk* Chunk::getChunk(const glm::vec3& _pos) const {
    return m_world->findChunk(Chunk::posToChunkPos(_pos));
}

Block* Chunk::findBlock(const glm::ivec3& _block_pos) const {
    return m_world->findBlock(_block_pos);
}

// https://stackoverflow.com/questions/55263298/draw-all-voxels-that-pass-through-a-3d-line-in-3d-voxel-space
void Chunk::findSolidBlocks(const glm::vec3& _start, const glm::vec3& _end, std::vector<const Block*>& blocks) const {
    glm::vec3 delta = glm::abs(_end - _start);
    float d_length = glm::length(delta);
    glm::vec3 tdelta = d_length / delta;
    glm::vec3 tmax = tdelta * 0.5f;

    glm::ivec3 current_block = Block::posToBlockPos(_start);
    glm::ivec3 end_block = Block::posToBlockPos(_end);
    glm::ivec3 step(
        current_block.x < _end.x   ? 1
        : current_block.x > _end.x ? -1
                                   : 0,
        current_block.y < _end.y   ? 1
        : current_block.y > _end.y ? -1
                                   : 0,
        current_block.z < _end.z   ? 1
        : current_block.z > _end.z ? -1
                                   : 0);

    glm::ivec3 block_delta = glm::abs(end_block - current_block);
    uint nb_blocks = block_delta.x + block_delta.y + block_delta.z;
    for (uint _ = 0; _ <= nb_blocks; _++) {
        glm::ivec3 current_chunk_pos = blockPosToChunkPos(current_block);
        if (m_pos != current_chunk_pos) {
            Chunk* chunk = m_world->findChunk(current_chunk_pos);
            if (chunk != nullptr) {
                chunk->findSolidBlocks(current_block, _end, blocks);
                return;
            }
        }
        const Block& block = getBlock(current_block);
        if (block.hasHitbox())
            blocks.push_back(&block);

        if (tmax.x <= tmax.y && tmax.x <= tmax.z) {
            current_block.x += step.x;
            tmax.x += tdelta.x;
        } else if (tmax.y <= tmax.z) {
            current_block.y += step.y;
            tmax.y += tdelta.y;
        } else {
            current_block.z += step.z;
            tmax.z += tdelta.z;
        }
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
            Block* block = &m_blocks[block_i];
            if (has_neighbour) {
                block->m_neighbours[_face_i] = nullptr;
            } else {
                Block* neighbour_block = &m_neighbours[_face_i]->m_blocks[neighbour_i];
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

void Chunk::generate(GenType _type) {
    glm::ivec3 world_pos;
    size_t block_i = 0;
    switch (_type) {
    case GenType::DEBUG_:
        for (world_pos.y = m_pos.y; world_pos.y < m_pos.y + CHUNK_SIZE; world_pos.y++) {
            for (world_pos.z = m_pos.z; world_pos.z < m_pos.z + CHUNK_SIZE; world_pos.z++) {
                for (world_pos.x = m_pos.x; world_pos.x < m_pos.x + CHUNK_SIZE; world_pos.x++) {
                    Block& block = m_blocks[block_i++];
                    block.getPos() = world_pos;
                    if (world_pos.y <= -45) {
                        block.getType() = BlockType::Air;
                    } else if (world_pos.y <= 0) {
                        block.getType() = BlockType::Stone;
                    } else if (world_pos.y <= 3) {
                        block.getType() = BlockType::Dirt;
                    } else if (world_pos.y <= 4) {
                        uint truc = (world_pos.y * 43 + world_pos.z) * 37 + world_pos.x;
                        block.getType() = BlockType(truc % BLOCK_TYPES_N);
                    } else {
                        block.getType() = BlockType::Air;
                    }
                    // block.getType() = world_pos.x % 2 == world_pos.y % 2 && world_pos.y % 2 == world_pos.z % 2 ? BlockType::Stone : BlockType::Air;
                }
            }
        }
        break;
    case GenType::SUPERFLAT:
        for (world_pos.y = m_pos.y; world_pos.y < m_pos.y + CHUNK_SIZE; world_pos.y++) {
            for (world_pos.z = m_pos.z; world_pos.z < m_pos.z + CHUNK_SIZE; world_pos.z++) {
                for (world_pos.x = m_pos.x; world_pos.x < m_pos.x + CHUNK_SIZE; world_pos.x++) {
                    Block& block = m_blocks[block_i++];
                    block.getPos() = world_pos;
                    if (world_pos.y <= -45) {
                        block.getType() = BlockType::Air;
                    } else if (world_pos.y <= 0) {
                        block.getType() = BlockType::Stone;
                    } else if (world_pos.y <= 3) {
                        block.getType() = BlockType::Dirt;
                    } else if (world_pos.y <= 4) {
                        block.getType() = BlockType::Grass;

                    } else {
                        block.getType() = BlockType::Air;
                    }
                }
            }
        }
        break;
    case GenType::OVERWORLD:
        for (world_pos.y = m_pos.y; world_pos.y < m_pos.y + CHUNK_SIZE; world_pos.y++) {
            for (world_pos.z = m_pos.z; world_pos.z < m_pos.z + CHUNK_SIZE; world_pos.z++) {
                for (world_pos.x = m_pos.x; world_pos.x < m_pos.x + CHUNK_SIZE; world_pos.x++) {
                    // const uint8_t ground_height = m_heightmap.value().getPixel(0, 0);
                    const uint8_t ground_height = m_heightmap.value().getPixelSafe(world_pos.x, world_pos.z);
                    // std::cout << ground_height << "\n";
                    Block& block = m_blocks[block_i++];
                    block.getPos() = world_pos;
                    if (world_pos.y <= -45) {
                        block.getType() = BlockType::Air;
                    } else if (world_pos.y <= ground_height) {
                        block.getType() = BlockType::Stone;
                    } else if (world_pos.y <= ground_height + 3) {
                        block.getType() = BlockType::Dirt;
                    } else if (world_pos.y <= ground_height + 4) {
                        int test = rand();
                        if (test < RAND_MAX / 4)
                            block.getType() = BlockType::IronBlock;
                        else
                            block.getType() = BlockType::Grass;

                    } else {
                        block.getType() = BlockType::Air;
                    }
                }
            }
        }
        break;
    default:
        throw std::runtime_error("ChunkGenType not supported in Chunk generation");
    }

    for (size_t i = 0; i < Chunk::NB_BLOCKS; i++)
        nb_translucent_block += (m_blocks[i].getTransparence() == BlockTransparence::TRANSLUCENT);
}

Chunk::Chunk(World* _world, const glm::ivec3& _chunk_pos, GenType _type) : m_world(_world), m_pos(_chunk_pos), m_aabb(glm::vec3(m_pos), glm::vec3(m_pos) + glm::vec3(CHUNK_SIZE)) {
    m_heightmap.emplace("ressources/textures/heightmap.png");
    generate(_type);
    initShaderData();
}

struct ChunkVertex {
    MathHelpers::u8pvec3 position;
    MathHelpers::i8pvec3 normal;
    MathHelpers::i8pvec3 tangent;
    MathHelpers::i8pvec3 bitangent;
    MathHelpers::fpvec2 uv;
};

// Si mes comptes sont bons on a 28.5MiB pour tout les chunks (c'est OK)
namespace ChunkMeshScratch {
std::array<ChunkVertex, Chunk::MAX_VERTICES> opaque_vertices;
std::array<MathHelpers::upvec3, Chunk::MAX_TRIANGLES> opaque_triangles;

std::array<ChunkVertex, Chunk::MAX_VERTICES> translucent_vertices;
std::array<MathHelpers::upvec3, Chunk::MAX_TRIANGLES> translucent_triangles;
std::array<float, Chunk::MAX_TRIANGLES / 2> translucent_quad_distances;
}; // namespace ChunkMeshScratch
void Chunk::initShaderData() {
    // OPAQUE DATA

    glGenVertexArrays(1, &m_opaque_VAO);
    glBindVertexArray(m_opaque_VAO);
    glGenBuffers(1, &m_opaque_VBO);
    glBindBuffer(GL_ARRAY_BUFFER, m_opaque_VBO);
    // Attribute 0: position
    glEnableVertexAttribArray(0);
    glVertexAttribIPointer(0, 3, GL_UNSIGNED_BYTE, sizeof(ChunkVertex), (void*)offsetof(ChunkVertex, position));
    // Attribute 1: normal
    glEnableVertexAttribArray(1);
    glVertexAttribIPointer(1, 3, GL_BYTE, sizeof(ChunkVertex), (void*)offsetof(ChunkVertex, normal));
    // Attribute 2: tangent
    glEnableVertexAttribArray(2);
    glVertexAttribIPointer(2, 3, GL_BYTE, sizeof(ChunkVertex), (void*)offsetof(ChunkVertex, tangent));
    // Attribute 3: bitangent
    glEnableVertexAttribArray(3);
    glVertexAttribIPointer(3, 3, GL_BYTE, sizeof(ChunkVertex), (void*)offsetof(ChunkVertex, bitangent));
    // Attribute 4: uv
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 2, GL_FLOAT, GL_FALSE, sizeof(ChunkVertex), (void*)offsetof(ChunkVertex, uv));
    // ELEMENT BUFFER
    glGenBuffers(1, &m_opaque_EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_opaque_EBO);

    // TRANSLUCENT DATA

    glGenVertexArrays(1, &m_translucent_VAO);
    glBindVertexArray(m_translucent_VAO);
    glGenBuffers(1, &m_translucent_VBO);
    glBindBuffer(GL_ARRAY_BUFFER, m_translucent_VBO);
    // Attribute 0: position
    glEnableVertexAttribArray(0);
    glVertexAttribIPointer(0, 3, GL_UNSIGNED_BYTE, sizeof(ChunkVertex), (void*)offsetof(ChunkVertex, position));
    // Attribute 1: normal
    glEnableVertexAttribArray(1);
    glVertexAttribIPointer(1, 3, GL_BYTE, sizeof(ChunkVertex), (void*)offsetof(ChunkVertex, normal));
    // Attribute 2: tangent
    glEnableVertexAttribArray(2);
    glVertexAttribIPointer(2, 3, GL_BYTE, sizeof(ChunkVertex), (void*)offsetof(ChunkVertex, tangent));
    // Attribute 3: bitangent
    glEnableVertexAttribArray(3);
    glVertexAttribIPointer(3, 3, GL_BYTE, sizeof(ChunkVertex), (void*)offsetof(ChunkVertex, bitangent));
    // Attribute 4: uv
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 2, GL_FLOAT, GL_FALSE, sizeof(ChunkVertex), (void*)offsetof(ChunkVertex, uv));
    // ELEMENT BUFFER
    glGenBuffers(1, &m_translucent_EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_translucent_EBO);
}

void Chunk::updateShaderData(const glm::vec3& _cam_pos) {
    m_opaque_vertices = m_opaque_triangles = m_translucent_vertices = m_translucent_triangles = 0;
    should_rebuild_mesh = false;

    glm::u8vec3 local_pos;
    int block_i = -1;
    for (local_pos.y = 0; local_pos.y < CHUNK_SIZE; local_pos.y++) {
        for (local_pos.z = 0; local_pos.z < CHUNK_SIZE; local_pos.z++) {
            for (local_pos.x = 0; local_pos.x < CHUNK_SIZE; local_pos.x++) {
                block_i++;
                Block& block = m_blocks[block_i];
                if (block.getType() == BlockType::Air) {
                    continue;
                }

                auto& vertices = block.getTransparence() == BlockTransparence::TRANSLUCENT ? ChunkMeshScratch::translucent_vertices : ChunkMeshScratch::opaque_vertices;
                auto& triangles = block.getTransparence() == BlockTransparence::TRANSLUCENT ? ChunkMeshScratch::translucent_triangles : ChunkMeshScratch::opaque_triangles;
                size_t& vertices_count = block.getTransparence() == BlockTransparence::TRANSLUCENT ? m_translucent_vertices : m_opaque_vertices;
                size_t& triangles_count = block.getTransparence() == BlockTransparence::TRANSLUCENT ? m_translucent_triangles : m_opaque_triangles;

                glm::vec3 block_center = glm::vec3(m_pos + glm::ivec3(local_pos)) + glm::vec3(0.5f);
                for (int face_i = 0; face_i < 6; face_i++) {
                    const Block* neighbour = block.m_neighbours[face_i];
                    if (neighbour != nullptr && (neighbour->getTransparence() == BlockTransparence::SOLID || block.getType() == neighbour->getType()))
                        continue;
                    const Block::FaceData& face = Block::FACE_DATA[face_i];

                    std::array<glm::vec2, 4> face_uvs = Block::getUV(atlas_dims, block.getType(), face_i);
                    for (int i = 0; i < 4; ++i) {
                        vertices[vertices_count].position = local_pos + face.vertices[i];
                        vertices[vertices_count].normal = face.normal;
                        vertices[vertices_count].uv = face_uvs[i];
                        vertices[vertices_count].tangent = face.tangent;
                        vertices[vertices_count].bitangent = face.bitangent;
                        vertices_count++;
                    }

                    glm::uvec3 offset(vertices_count - 4);
                    if (block.getTransparence() == BlockTransparence::TRANSLUCENT) {
                        glm::vec3 face_centroid = block_center + 0.5f * glm::vec3(face.normal);
                        float distance_to_cam = glm::distance(_cam_pos, face_centroid);

                        size_t i = triangles_count / 2;
                        while (i > 0 && ChunkMeshScratch::translucent_quad_distances[i - 1] < distance_to_cam) {
                            triangles[i * 2] = triangles[i * 2 - 2];
                            triangles[i * 2 + 1] = triangles[i * 2 - 1];
                            ChunkMeshScratch::translucent_quad_distances[i] = ChunkMeshScratch::translucent_quad_distances[i - 1];
                            i--;
                        }
                        triangles[i * 2] = face.triangles[0] + offset;
                        triangles[i * 2 + 1] = face.triangles[1] + offset;
                        ChunkMeshScratch::translucent_quad_distances[i] = distance_to_cam;
                        triangles_count += 2;
                    } else {
                        triangles[triangles_count++] = face.triangles[0] + offset;
                        triangles[triangles_count++] = face.triangles[1] + offset;
                    }
                }
            }
        }
    }

    glBindVertexArray(m_opaque_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_opaque_VBO);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(m_opaque_vertices * sizeof(ChunkVertex)), ChunkMeshScratch::opaque_vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_opaque_EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(m_opaque_triangles * sizeof(MathHelpers::upvec3)), ChunkMeshScratch::opaque_triangles.data(), GL_STATIC_DRAW);

    glBindVertexArray(m_translucent_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_translucent_VBO);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(m_translucent_vertices * sizeof(ChunkVertex)), ChunkMeshScratch::translucent_vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_translucent_EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(m_translucent_triangles * sizeof(MathHelpers::upvec3)), ChunkMeshScratch::translucent_triangles.data(), GL_STATIC_DRAW);
}

void Chunk::renderOpaque() const {
    glBindVertexArray(m_opaque_VAO);
    glDrawElements(GL_TRIANGLES, m_opaque_triangles * 3, GL_UNSIGNED_INT, 0);
}

void Chunk::renderTranslucent() const {
    glBindVertexArray(m_translucent_VAO);
    glDrawElements(GL_TRIANGLES, m_translucent_triangles * 3, GL_UNSIGNED_INT, 0);
}

void Chunk::clearShaderData() {
    if (m_opaque_VAO) {
        glDeleteVertexArrays(1, &m_opaque_VAO);
        m_opaque_VAO = 0;
    }
    if (m_opaque_VBO) {
        glDeleteBuffers(1, &m_opaque_VBO);
        m_opaque_VBO = 0;
    }
    if (m_opaque_EBO) {
        glDeleteBuffers(1, &m_opaque_EBO);
        m_opaque_EBO = 0;
    }

    if (m_translucent_VAO) {
        glDeleteVertexArrays(1, &m_translucent_VAO);
        m_translucent_VAO = 0;
    }
    if (m_translucent_VBO) {
        glDeleteBuffers(1, &m_translucent_VBO);
        m_translucent_VBO = 0;
    }
    if (m_translucent_EBO) {
        glDeleteBuffers(1, &m_translucent_EBO);
        m_translucent_EBO = 0;
    }
}
