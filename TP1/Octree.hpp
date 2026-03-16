#pragma once

// Include GLEW
#include <GL/glew.h>

// Include GLM
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vector>

struct OctreeData {
    size_t element_count;
    std::vector<size_t> vertices_indices;
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec3> normals;
    glm::vec3 representant_position;
    glm::vec3 representant_normal;

    OctreeData() : element_count(0), representant_position(glm::vec3(0., 0., 0.)), representant_normal(glm::vec3(0., 0., 0.)) {}

    void addElement(size_t index, glm::vec3 posistion, glm::vec3 normal) {
        vertices_indices.push_back(index);
        vertices.push_back(posistion);
        normals.push_back(normal);
        element_count++;
    }

    void calcRepresentants() {
        if (element_count == 0) {
            return;
        }

        for (glm::vec3 v : vertices) {
            representant_position += v;
        }
        representant_position /= vertices.size();

        for (glm::vec3 n : normals) {
            representant_normal += n;
        }
        representant_normal /= normals.size();
    }

    void clear() {
        element_count = 0;
        vertices_indices.resize(0);
        vertices.resize(0);
        normals.resize(0);
    }
};

class Octree {
protected:
    size_t max_vert_per_leaf;
    glm::vec3 min_pos, max_pos;

    bool is_leaf;
    OctreeData data;
    std::vector<Octree> children;

    GLuint m_VAO = 0;
    GLuint m_vertices_VBO = 0;
    GLuint m_lines_EBO = 0;

    // divide the current node and return the number of children that have at least 1 vertex.
    void divide() {
        if (!is_leaf) {
            return;
        }
        is_leaf = false;

        // create the children
        for (size_t x = 0; x < 2; x++) {
            for (size_t y = 0; y < 2; y++) {
                for (size_t z = 0; z < 2; z++) {
                    glm::vec3 temp = (max_pos - min_pos) / 2.f;
                    glm::vec3 child_min_pos = min_pos + glm::vec3(x, y, z) * temp;
                    glm::vec3 child_max_pos = child_min_pos + temp;
                    children.push_back(Octree(max_vert_per_leaf, child_min_pos, child_max_pos));
                }
            }
        }

        // put the vertices in the children
        for (size_t i = 0; i < data.element_count; i++) {
            size_t vi = data.vertices_indices[i];
            glm::vec3 v = data.vertices[i];
            glm::vec3 n = data.normals[i];
            size_t index = getIndex(v);
            children[index].pushVertex(vi, v, n);
        }
        data = OctreeData();
    }

    size_t getCellX(glm::vec3 pos) { return 2 * (pos[0] - min_pos[0]) / (max_pos[0] - min_pos[0]); }
    size_t getCellY(glm::vec3 pos) { return 2 * (pos[1] - min_pos[1]) / (max_pos[1] - min_pos[1]); }
    size_t getCellZ(glm::vec3 pos) { return 2 * (pos[2] - min_pos[2]) / (max_pos[2] - min_pos[2]); }
    size_t getIndex(glm::vec3 pos) { return getCellX(pos) * 4 + getCellY(pos) * 2 + getCellZ(pos); }
    bool isInside(glm::vec3 const &v) {
        return min_pos[0] - FLT_EPSILON <= v[0] && v[0] <= max_pos[0] + FLT_EPSILON &&
               min_pos[1] - FLT_EPSILON <= v[1] && v[1] <= max_pos[1] + FLT_EPSILON &&
               min_pos[2] - FLT_EPSILON <= v[2] && v[2] <= max_pos[2] + FLT_EPSILON;
    }

public:
    Octree() : max_vert_per_leaf(), min_pos(), max_pos(), is_leaf(true), data(), children({}) {}
    Octree(size_t const &max_vert_per_leaf, glm::vec3 const &min_pos, glm::vec3 const &max_pos) : max_vert_per_leaf(max_vert_per_leaf), min_pos(min_pos), max_pos(max_pos), is_leaf(true), data(), children({}) {}

    // recalculates the Representants of all the tree
    void calcRepresentants() {
        if (data.element_count > 0) {
            data.calcRepresentants();
        } else if (!is_leaf) {
            for (Octree &child : children) {
                child.calcRepresentants();
            }
        }
    }

    // Push a vertex, normal and its index in the old std::vector in the tree
    void pushVertex(size_t vi, glm::vec3 v, glm::vec3 n) {
        if (!isInside(v)) {
            return;
        }

        if (!is_leaf) {
            children[getIndex(v)].pushVertex(vi, v, n);
        } else {
            data.addElement(vi, v, n);
            if (data.element_count > max_vert_per_leaf) {
                divide();
            }
        }
    }

    // Exctract the Octree data in 3 separate std::vectors
    void fillRepresentantsData(std::vector<std::vector<size_t>> &vertices_indices, std::vector<glm::vec3> &vertices, std::vector<glm::vec3> &normals) const {
        if (!is_leaf) {
            for (size_t i = 0; i < 8; i++) {
                children[i].fillRepresentantsData(vertices_indices, vertices, normals);
            }
        } else if (data.element_count > 0) {
            std::vector<size_t> indices;
            for (size_t i = 0; i < data.element_count; i++) {
                indices.push_back(data.vertices_indices[i]);
            }
            vertices_indices.push_back(indices);
            vertices.push_back(data.representant_position);
            normals.push_back(data.representant_normal);
        }
    }

    void initShaderData() {
        if (!is_leaf) {
            for (size_t i = 0; i < 8; i++) {
                children[i].initShaderData();
            }
        } else if (data.element_count > 0) {
            glGenVertexArrays(1, &m_VAO);
            glBindVertexArray(m_VAO);

            std::vector<glm::vec3> m_vertices{
                glm::vec3(min_pos[0], min_pos[1], min_pos[2]),
                glm::vec3(min_pos[0], min_pos[1], max_pos[2]),
                glm::vec3(min_pos[0], max_pos[1], min_pos[2]),
                glm::vec3(min_pos[0], max_pos[1], max_pos[2]),
                glm::vec3(max_pos[0], min_pos[1], min_pos[2]),
                glm::vec3(max_pos[0], min_pos[1], max_pos[2]),
                glm::vec3(max_pos[0], max_pos[1], min_pos[2]),
                glm::vec3(max_pos[0], max_pos[1], max_pos[2])};

            glGenBuffers(1, &m_vertices_VBO);
            glBindBuffer(GL_ARRAY_BUFFER, m_vertices_VBO);
            glBufferData(GL_ARRAY_BUFFER, m_vertices.size() * sizeof(glm::vec3), m_vertices.data(), GL_STATIC_DRAW);
            glEnableVertexAttribArray(0);
            glBindBuffer(GL_ARRAY_BUFFER, m_vertices_VBO);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

            std::vector<glm::uvec2> m_lines{
                // Z
                glm::uvec2(0, 1),
                glm::uvec2(2, 3),
                glm::uvec2(4, 5),
                glm::uvec2(6, 7),

                // Y
                glm::uvec2(0, 2),
                glm::uvec2(1, 3),
                glm::uvec2(4, 6),
                glm::uvec2(5, 7),

                // X
                glm::uvec2(0, 4),
                glm::uvec2(1, 5),
                glm::uvec2(2, 6),
                glm::uvec2(3, 7)};

            glGenBuffers(1, &m_lines_EBO);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_lines_EBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_lines.size() * sizeof(glm::uvec3), m_lines.data(), GL_STATIC_DRAW);
            glBindBuffer(GL_ARRAY_BUFFER, 0);

            glBindVertexArray(0);
        }
    }

    void render() const {
        if (!is_leaf) {
            for (size_t i = 0; i < 8; i++) {
                children[i].render();
            }
        } else if (data.element_count > 0) {
            glBindVertexArray(m_VAO);
            glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, 0);
        }
    }

    void clear() {
        if (!is_leaf) {
            for (size_t i = 0; i < 8; i++) {
                children[i].clear();
            }
        } else if (data.element_count > 0) {
            data.clear();
        }
    }
};
