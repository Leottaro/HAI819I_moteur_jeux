#pragma once

// Include GLM
#include <glm/glm.hpp>

// USUAL INCLUDES
#include <vector>
#include "AABB.hpp"

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
    AABB<float> aabb;

    bool is_leaf;
    OctreeData data;
    std::vector<Octree> children;

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
                    glm::vec3 temp = (aabb.max - aabb.min) / 2.f;
                    glm::vec3 child_min_pos = aabb.min + glm::vec3(x, y, z) * temp;
                    glm::vec3 child_max_pos = child_min_pos + temp;
                    children.push_back(Octree(max_vert_per_leaf, AABB(child_min_pos, child_max_pos)));
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

    size_t getCellX(glm::vec3 pos) { return 2 * (pos.x - aabb.min.x) / (aabb.max.x - aabb.min.x); }
    size_t getCellY(glm::vec3 pos) { return 2 * (pos.y - aabb.min.y) / (aabb.max.y - aabb.min.y); }
    size_t getCellZ(glm::vec3 pos) { return 2 * (pos.z - aabb.min.z) / (aabb.max.z - aabb.min.z); }
    size_t getIndex(glm::vec3 pos) { return getCellX(pos) * 4 + getCellY(pos) * 2 + getCellZ(pos); }
    bool isInside(glm::vec3 const &v) {
        return aabb.min.x - FLT_EPSILON <= v.x && v.x <= aabb.max.x + FLT_EPSILON &&
               aabb.min.y - FLT_EPSILON <= v.y && v.y <= aabb.max.y + FLT_EPSILON &&
               aabb.min.z - FLT_EPSILON <= v.z && v.z <= aabb.max.z + FLT_EPSILON;
    }

public:
    Octree() : max_vert_per_leaf(), aabb(), is_leaf(true), data(), children({}) {}
    Octree(const size_t &max_vert_per_leaf, const AABB<float> &aabb) : max_vert_per_leaf(max_vert_per_leaf), aabb(aabb), is_leaf(true), data(), children({}) {}

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
            aabb.initShaderData();
        }
    }

    void render() const {
        if (!is_leaf) {
            for (size_t i = 0; i < 8; i++) {
                children[i].render();
            }
        } else if (data.element_count > 0) {
            aabb.render();
        }
    }

    void clear() {
        if (!is_leaf) {
            for (size_t i = 0; i < 8; i++) {
                children[i].clear();
            }
        } else if (data.element_count > 0) {
            aabb.clearShaderData();
            data.clear();
        }
    }
};
