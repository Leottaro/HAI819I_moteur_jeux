// USUAL INCLUDES
#include "Chunk.hpp"

#include "World.hpp"
#include "objects/blocks.hpp"
#include "objects/structures.hpp"
#include "objects/textures.hpp"
#include <stb_perlin.h>
#include <stdexcept>

Chunk* Chunk::getChunk(const glm::vec3& _pos) const {
    return m_world->findChunk(Chunk::posToChunkPos(_pos));
}

Block* Chunk::findBlock(const glm::ivec3& _block_pos) const {
    return m_world->findBlock(_block_pos);
}

constexpr int ONE_IN_TEN = RAND_MAX / 10;
constexpr int ONE_IN_HUNDRED = RAND_MAX / 100;
constexpr int ONE_IN_THOUSAND = RAND_MAX / 1000;

// http://www.cse.yorku.ca/~amana/research/grid.pdf
std::vector<const Block*> Chunk::findBlockLine(const glm::vec3& _start, const glm::vec3& _end) const {
    glm::vec3 dir = _end - _start;
    float d_length = glm::length(dir);

    glm::ivec3 start_block_pos{Block::posToBlockPos(_start)};
    glm::ivec3 end_block_pos{Block::posToBlockPos(_end)};
    const Block* current_block = m_pos == blockPosToChunkPos(start_block_pos) ? &getBlock(start_block_pos) : m_world->findBlock(start_block_pos);

    std::vector<const Block*> blocks;
    if (d_length <= 0.f && current_block == nullptr)
        return blocks;
    blocks.push_back(current_block);

    // How far along the ray to cross one full voxel on each axis.
    // Infinity when the ray has no component on that axis (never crosses).
    constexpr float INF = std::numeric_limits<float>::infinity();
    glm::vec3 delta = glm::abs(dir);
    glm::vec3 tdelta(delta.x > 0.0f ? d_length / delta.x : INF, delta.y > 0.0f ? d_length / delta.y : INF, delta.z > 0.0f ? d_length / delta.z : INF);

    // Step direction but in face orientation, so we can use block neighbour for traversal
    glm::ivec3 step_face(dir.x > 0.0f   ? 4 // (+X)
                         : dir.x < 0.0f ? 1 // (-X)
                                        : INF,
                         dir.y > 0.0f   ? 5 // (+Y)
                         : dir.y < 0.0f ? 2 // (-Y)
                                        : INF,
                         dir.z > 0.0f   ? 3 // (+Z)
                         : dir.z < 0.0f ? 0 // (-Z)
                                        : INF);

    glm::vec3 tmax;
    for (int i = 0; i < 3; i++) {
        if (step_face[i] == INF) {
            tmax[i] = INF;
            continue;
        }

        float frac = dir[i] > 0.0f ? (float(start_block_pos[i]) + 1.0f) - _start[i] // distance to next wall ahead
                                   : _start[i] - float(start_block_pos[i]);         // distance to wall behind
        if (frac <= 0.0f)
            frac = 1.0f; // exactly on boundary: treat as a full voxel ahead
        tmax[i] = frac * tdelta[i];
    }

    do {
        if (tmax.x <= tmax.y && tmax.x <= tmax.z) {
            current_block = current_block->m_neighbours[step_face.x];
            tmax.x += tdelta.x;
        } else if (tmax.y <= tmax.z) {
            current_block = current_block->m_neighbours[step_face.y];
            tmax.y += tdelta.y;
        } else {
            current_block = current_block->m_neighbours[step_face.z];
            tmax.z += tdelta.z;
        }

        if (current_block == nullptr)
            return blocks;
        blocks.push_back(current_block);

    } while (current_block->getPos() != end_block_pos);

    return blocks;
}

void Chunk::updateBlockNeighbours(uint8_t _face_i) {
    uint8_t face_axis = _face_i % 3;
    uint8_t face_depth = _face_i / 3;
    int block_i = 0;
    int neighbour_i = face_axis == 0   ? CHUNK_SIZE * (CHUNK_SIZE - 1)
                      : face_axis == 1 ? CHUNK_SIZE * CHUNK_SIZE * (CHUNK_SIZE - 1)
                                       : CHUNK_SIZE - 1;
    int i_step = face_axis == 0   ? 1
                 : face_axis == 1 ? 1
                                  : CHUNK_SIZE;
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
    std::vector<glm::u8vec3> surface_blocks;
    size_t block_i = 0;
    constexpr uint32_t MOUTAINT_HEIGHT = 10.f;
    switch (_type) {
    case GenType::DEBUG_:
        for (world_pos.x = m_pos.x; world_pos.x < m_pos.x + CHUNK_SIZE; world_pos.x++) {
            for (world_pos.z = m_pos.z; world_pos.z < m_pos.z + CHUNK_SIZE; world_pos.z++) {
                for (world_pos.y = m_pos.y; world_pos.y < m_pos.y + CHUNK_SIZE; world_pos.y++) {
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
        for (world_pos.x = m_pos.x; world_pos.x < m_pos.x + CHUNK_SIZE; world_pos.x++) {
            for (world_pos.z = m_pos.z; world_pos.z < m_pos.z + CHUNK_SIZE; world_pos.z++) {
                for (world_pos.y = m_pos.y; world_pos.y < m_pos.y + CHUNK_SIZE; world_pos.y++) {
                    Block& block = m_blocks[block_i++];
                    block.getPos() = world_pos;
                    block.getType() = BlockType::Air;
                    // if (world_pos.y <= -45) {
                    //     block.getType() = BlockType::Air;
                    // } else if (world_pos.y <= 0) {
                    //     block.getType() = BlockType::Stone;
                    // } else if (world_pos.y <= 3) {
                    //     block.getType() = BlockType::Dirt;
                    // } else if (world_pos.y <= 4) {
                    //     block.getType() = BlockType::Grass;
                    // } else {
                    //     block.getType() = BlockType::Air;
                    // }
                }
            }
        }
        break;
    case GenType::OVERWORLD:
        surface_blocks.reserve(CHUNK_SIZE * CHUNK_SIZE);
        for (world_pos.x = m_pos.x; world_pos.x < m_pos.x + CHUNK_SIZE; world_pos.x++) {
            for (world_pos.z = m_pos.z; world_pos.z < m_pos.z + CHUNK_SIZE; world_pos.z++) {
                glm::vec3 world_float = glm::vec3(world_pos) / Chunk::CHUNK_SIZE;
                const float perlin_height = stb_perlin_noise3_seed(world_float.x, 0.f, world_float.z, 0.f, 0.f, 0., m_world->getWorldSeed());
                const int ground_height = static_cast<int>((perlin_height + 1) * MOUTAINT_HEIGHT);
                for (world_pos.y = m_pos.y; world_pos.y < m_pos.y + CHUNK_SIZE; world_pos.y++) {
                    // std::cout << world_pos << "/" << ground_height << "\n";
                    Block& block = m_blocks[block_i++];
                    block.getPos() = world_pos;
                    if (world_pos.y <= -100) {
                        block.getType() = BlockType::Air;
                    } else if (world_pos.y <= -99) {
                        block.getType() = BlockType::PierreDeLit;
                    } else if (world_pos.y <= ground_height) {
                        if (rand() < ONE_IN_THOUSAND)
                            block.getType() = BlockType::DiamondOre;
                        else
                            block.getType() = BlockType::Stone;
                    } else if (world_pos.y <= ground_height + 3) {
                        block.getType() = BlockType::Dirt;
                    } else if (world_pos.y <= ground_height + 4) {
                        block.getType() = BlockType::Grass;
                        surface_blocks.push_back(world_pos - m_pos);
                    } else {
                        block.getType() = BlockType::Air;
                    }
                }
            }
        }
        for (const glm::u8vec3& surface_pos : surface_blocks) {
            if (rand() >= TREE_CHANCE)
                continue;

            for (const auto& [block_type, struct_pos] : TREE_DATA) {
                glm::u8vec3 block_pos = surface_pos + glm::u8vec3(struct_pos);
                if (block_pos.x >= CHUNK_SIZE || block_pos.y >= CHUNK_SIZE || block_pos.z >= CHUNK_SIZE)
                    continue;

                // std::cout << "block de type: " << block_names[static_cast<size_t>(block_type)] << " à la position " << block_pos << " de " << m_pos << "\n";
                Block& block = m_blocks[posToBlockI(block_pos)];
                block.getType() = block_type;
            }
        }
        break;
    default:
        throw std::runtime_error("ChunkGenType not supported in Chunk generation");
    }
}

Chunk::Chunk(World* _world, const glm::ivec3& _chunk_pos, GenType _type) : m_world(_world), m_pos(_chunk_pos), m_aabb(glm::vec3(m_pos), glm::vec3(m_pos) + glm::vec3(CHUNK_SIZE)) {
    // m_heightmap.emplace("ressources/textures/heightmap.png");
    generate(_type);
}

// -------------------------------------------------------------------------
// CHUNK RENDERER
// -------------------------------------------------------------------------

void ChunkRenderer::initShaderData() {
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

void ChunkRenderer::updateShaderData(const glm::vec3& _cam_pos) {
    m_should_rebuild_mesh = false;
    m_opaque_vertices = m_opaque_triangles = m_translucent_vertices = m_translucent_triangles = 0;

    glm::u8vec3 local_pos;
    int block_i = -1;
    for (local_pos.x = 0; local_pos.x < Chunk::CHUNK_SIZE; local_pos.x++) {
        for (local_pos.z = 0; local_pos.z < Chunk::CHUNK_SIZE; local_pos.z++) {
            for (local_pos.y = 0; local_pos.y < Chunk::CHUNK_SIZE; local_pos.y++) {
                block_i++;
                const Block& block = m_chunk->m_blocks[block_i];
                if (block.getType() == BlockType::Air) {
                    continue;
                }

                auto& vertices = block.getTransparence() == BlockTransparency::TRANSLUCENT ? translucent_vertices : opaque_vertices;
                auto& triangles = block.getTransparence() == BlockTransparency::TRANSLUCENT ? translucent_triangles : opaque_triangles;
                size_t& vertices_count = block.getTransparence() == BlockTransparency::TRANSLUCENT ? m_translucent_vertices : m_opaque_vertices;
                size_t& triangles_count = block.getTransparence() == BlockTransparency::TRANSLUCENT ? m_translucent_triangles : m_opaque_triangles;

                glm::vec3 block_center = glm::vec3(m_chunk->m_pos + glm::ivec3(local_pos)) + glm::vec3(0.5f);
                for (int face_i = 0; face_i < 6; face_i++) {
                    const Block* neighbour = block.m_neighbours[face_i];
                    if (neighbour != nullptr &&
                        (neighbour->getTransparence() == BlockTransparency::SOLID || (block.getType() == neighbour->getType() && block.getTransparence() != BlockTransparency::SEMI_TRANSPARENT)))
                        continue;
                    const Block::FaceData& face = Block::FACE_DATA[face_i];

                    std::array<glm::vec2, 4> face_uvs = Block::getUV(block.getType(), face_i);
                    for (int i = 0; i < 4; ++i) {
                        vertices[vertices_count].position = local_pos + face.vertices[i];
                        vertices[vertices_count].normal = face.normal;
                        vertices[vertices_count].uv = face_uvs[i];
                        vertices[vertices_count].tangent = face.tangent;
                        vertices[vertices_count].bitangent = face.bitangent;
                        vertices_count++;
                    }

                    glm::uvec3 offset(vertices_count - 4);
                    if (block.getTransparence() == BlockTransparency::TRANSLUCENT) {
                        glm::vec3 face_centroid = block_center + 0.5f * glm::vec3(face.normal);
                        float distance_to_cam2 = glm::distance2(_cam_pos, face_centroid);

                        size_t i = triangles_count / 2;
                        while (i > 0 && translucent_quad_distances2[i - 1] < distance_to_cam2) {
                            triangles[i * 2] = triangles[i * 2 - 2];
                            triangles[i * 2 + 1] = triangles[i * 2 - 1];
                            translucent_quad_distances2[i] = translucent_quad_distances2[i - 1];
                            i--;
                        }
                        triangles[i * 2] = face.triangles[0] + offset;
                        triangles[i * 2 + 1] = face.triangles[1] + offset;
                        translucent_quad_distances2[i] = distance_to_cam2;
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
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(m_opaque_vertices * sizeof(ChunkVertex)), opaque_vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_opaque_EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(m_opaque_triangles * sizeof(MathHelpers::upvec3)), opaque_triangles.data(), GL_STATIC_DRAW);

    glBindVertexArray(m_translucent_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_translucent_VBO);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(m_translucent_vertices * sizeof(ChunkVertex)), translucent_vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_translucent_EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(m_translucent_triangles * sizeof(MathHelpers::upvec3)), translucent_triangles.data(), GL_STATIC_DRAW);
}

void ChunkRenderer::renderOpaque() const {
    glBindVertexArray(m_opaque_VAO);
    glDrawElements(GL_TRIANGLES, m_opaque_triangles * 3, GL_UNSIGNED_INT, 0);
}

void ChunkRenderer::renderTranslucent() const {
    glBindVertexArray(m_translucent_VAO);
    glDrawElements(GL_TRIANGLES, m_translucent_triangles * 3, GL_UNSIGNED_INT, 0);
}

void ChunkRenderer::clearShaderData() {
    if (m_opaque_VAO) {
        gl_global_context.addArrayToDelete(m_opaque_VAO);
        m_opaque_VAO = 0;
    }
    if (m_opaque_VBO) {
        gl_global_context.addBufferToDelete(m_opaque_VBO);
        m_opaque_VBO = 0;
    }
    if (m_opaque_EBO) {
        gl_global_context.addBufferToDelete(m_opaque_EBO);
        m_opaque_EBO = 0;
    }

    if (m_translucent_VAO) {
        gl_global_context.addArrayToDelete(m_translucent_VAO);
        m_translucent_VAO = 0;
    }
    if (m_translucent_VBO) {
        gl_global_context.addBufferToDelete(m_translucent_VBO);
        m_translucent_VBO = 0;
    }
    if (m_translucent_EBO) {
        gl_global_context.addBufferToDelete(m_translucent_EBO);
        m_translucent_EBO = 0;
    }
}