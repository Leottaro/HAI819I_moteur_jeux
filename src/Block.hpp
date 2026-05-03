#pragma once

// GLM
#define GLM_FORCE_CONSTEXPR
#include <glm/glm.hpp>

// USUAL INCLUDES
#include "objects/blocks.hpp"

constexpr std::array<int, 6> OPPOSITE_FACE{3, 4, 5, 0, 1, 2};

class Block {
private:
    BlockType m_type;
    glm::ivec3 m_pos;

public:
    std::array<Block *, 6> m_neighbours{nullptr};

    Block() : m_type(BlockType::Air) {}
    Block(BlockType _type, const glm::ivec3 &_pos) : m_type(_type), m_pos(_pos) {}

    inline const BlockType &getType() const { return m_type; }
    inline BlockType &getType() { return m_type; }
    inline const glm::ivec3 &getPos() const { return m_pos; }
    inline glm::ivec3 &getPos() { return m_pos; }

    inline bool isTransparent() const { return m_type == BlockType::Air || m_type == BlockType::Glass; }

    static std::array<glm::vec2, 4> getUV(BlockType block_type, int face_i) {
        if (block_type == BlockType::Air) {
            return {{glm::vec2(0), glm::vec2(0), glm::vec2(0), glm::vec2(0)}};
        }
        
        size_t type_idx = static_cast<size_t>(block_type) - 1;
        const auto& uv_base = UB_TABLE_DATA[type_idx][face_i];
        
        float u_idx = uv_base[0];
        float v_idx = uv_base[1];
        
        // Convertir en coordonnées UV OpenGL (atlas 4x4)
        constexpr float atlas_inv = 1.0f / 4.0f;
        return {{
            glm::vec2((u_idx) * atlas_inv, (v_idx + 1) * atlas_inv),
            glm::vec2((u_idx + 1) * atlas_inv, (v_idx + 1) * atlas_inv),
            glm::vec2((u_idx + 1) * atlas_inv, (v_idx) * atlas_inv),
            glm::vec2((u_idx) * atlas_inv, (v_idx) * atlas_inv),
        }};
    }
};