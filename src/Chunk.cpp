// USUAL INCLUDES
#include "Chunk.hpp"
#include <stdexcept>

Chunk::Chunk(const glm::ivec3 &_chunk_pos, GenType _type) : m_pos(_chunk_pos) {
    switch (_type) {
    case GenType::SUPERFLAT:
        foreachBlockWorld([](int world_x, int world_y, int world_z, Block::BlockType &type) {
            if (world_y <= -1)
                type = Block::BlockType::Air;
            else if (world_y <= 0)
                type = Block::BlockType::Bedrock;
            else if (world_y <= 3)
                type = Block::BlockType::Dirt;
            else if (world_y <= 4)
                type = Block::BlockType::Grass;
            else
                type = Block::BlockType::Air;
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

    foreachBlockWorld([&positions, &normals, &triangles, &uvs](int world_x, int world_y, int world_z, Block::BlockType &type) {
        if (type == Block::BlockType::Air)
            return;
        glm::vec3 pos(world_x, world_y, world_z);
        for (int face_i = 0; face_i < 6; face_i++) {
            const Block::FaceData &face = Block::FACE_DATA[face_i];
            for (int i = 0; i < 4; ++i) {
                positions.push_back(pos + face.vertices[i]);
                normals.push_back(face.normal);
            }
            glm::uvec3 offset(positions.size() - 4);
            triangles.push_back(face.triangles[0] + offset);
            triangles.push_back(face.triangles[1] + offset);
        }
    });
    uvs.resize(positions.size());
    m_mesh.initShaderData();
}
