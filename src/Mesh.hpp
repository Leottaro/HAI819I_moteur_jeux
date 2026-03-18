#pragma once

// GLEW
#include <GL/glew.h>

// GLM
#include <glm/glm.hpp>
#include <glm/ext.hpp>

// USUAL INCLUDES
#include <string>
#include <vector>
#include "./ImageBase.h"
#include "./Octree.hpp"

class Mesh {
private:
    std::vector<glm::vec3> m_vertices = std::vector<glm::vec3>();
    std::vector<glm::vec3> m_normals = std::vector<glm::vec3>();
    std::vector<glm::vec2> m_uvs = std::vector<glm::vec2>();
    std::vector<glm::uvec3> m_triangles = std::vector<glm::uvec3>();
    Octree m_octree;

    GLuint m_VAO = 0;
    GLuint m_vertices_VBO = 0;
    GLuint m_normals_VBO = 0;
    GLuint m_uvs_VBO = 0;
    GLuint m_triangles_EBO = 0;

    void centerAndScaleToUnit();

public:
    virtual ~Mesh();

    // INITIALIZERS
    Mesh() {}
    Mesh(const std::string &filename) { loadOFF(filename); }
    void loadOFF(const std::string &filename);
    void setSingleTriangle();
    void setSimpleGrid(const glm::uvec2 &_resolution);                                           // Create a grid where x and z varies in [0;1]
    void setSimpleTerrain(const glm::uvec2 &_resolution, glm::vec2 y_range = glm::vec2(0., 1.)); // Create a terrain where x and z varies in [0;1] and y varies in y_range
    void setSimpleTerrain(const glm::uvec2 &_resolution, const ImageBase &_heightmap);           // Create a terrain where x and z varies in [0;1] and y varies in the heightmap
    void setCube(size_t _n);                                                                     // Create a cube where x, y and z varies in [0;1]
    void setCubeSphere(size_t _n);                                                               // Create a CubeSphere of center (0,0,0) and radius 1
    void setSphere(size_t nTheta, size_t nPhi);

    // GETTERS
    inline size_t nbVertices() const { return m_vertices.size(); }
    inline size_t nbTriangles() const { return m_triangles.size(); }
    inline const std::vector<glm::vec3> &vertexPositions() const { return m_vertices; }
    inline std::vector<glm::vec3> &vertexPositions() { return m_vertices; }
    inline const std::vector<glm::vec3> &vertexNormals() const { return m_normals; }
    inline std::vector<glm::vec3> &vertexNormals() { return m_normals; }
    inline const std::vector<glm::vec2> &vertexTexCoords() const { return m_uvs; }
    inline std::vector<glm::vec2> &vertexTexCoords() { return m_uvs; }
    inline const std::vector<glm::uvec3> &triangleIndices() const { return m_triangles; }
    inline std::vector<glm::uvec3> &triangleIndices() { return m_triangles; }

    /// Compute the parameters of a sphere which bounds the mesh
    void computeBoundingSphere(glm::vec3 &center, float &radius) const;

    void recomputePerVertexNormals(bool angleBased = false);
    void recomputePerVertexTextureCoordinates();

    // TP4
    std::pair<glm::vec3, size_t> computeheight(const glm::uvec2 &_grid_resolution, float _x, float _z) const;
    std::pair<glm::vec3, glm::vec3> computeheight(const glm::uvec2 &_grid_resolution, const glm::mat4 &_transfo, const glm::vec3 &_p) const;
    Mesh adaptiveSimplify(size_t max_vert_per_leaf) const;

    void initShaderData();
    void render() const;
    void renderOctree() const;
    void clear();
};
