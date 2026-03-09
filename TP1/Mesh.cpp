#define _USE_MATH_DEFINES

#include "Mesh.hpp"
#include "Transformation.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cmath>

using namespace std;

Mesh::~Mesh() {
    clear();
}

void Mesh::centerAndScaleToUnit() {
    glm::vec3 center(0.);
    for (unsigned int i = 0; i < m_vertices.size(); i++)
        center += m_vertices[i];
    center /= m_vertices.size();

    float maxD = distance(m_vertices[0], center);
    for (unsigned int i = 1; i < m_vertices.size(); i++) {
        float m = distance(m_vertices[i], center);
        if (m > maxD)
            maxD = m;
    }
    for (unsigned int i = 0; i < m_vertices.size(); i++)
        m_vertices[i] = (m_vertices[i] - center) / maxD;
}

void Mesh::loadOFF(const std::string &filename) {
    ifstream in(filename.c_str());
    if (!in)
        return;
    string offString;
    unsigned int sizeV, sizeT, tmp;
    in >> offString >> sizeV >> sizeT >> tmp;
    // cout << "loading mesh at \"" << filename << "\"" << endl
    //      << "siseV: " << sizeV << endl
    //      << "sizeT: " << sizeT << endl;

    m_vertices.resize(sizeV);
    for (unsigned int i = 0; i < sizeV; i++) {
        in >> m_vertices[i][0] >> m_vertices[i][1] >> m_vertices[i][2];
        // cout << "position: " << m_vertices[i][0] << "," << m_vertices[i][1] << "," << m_vertices[i][2];

        while (in.peek() == ' ')
            in.get();

        if (!(in.peek() == '\n' || in.peek() == '\r' || in.eof())) {
            in >> m_normals[i][0] >> m_normals[i][1] >> m_normals[i][2];
            // cout << ", normal: " << m_normals[i][0] << "," << m_normals[i][1] << "," << m_normals[i][2];
        }
        // cout << endl;

        while (in.peek() == ' ')
            in.get();
    }

    int s;
    m_triangles.resize(sizeT);
    for (unsigned int i = 0; i < sizeT; i++) {
        in >> s;
        in >> m_triangles[i][0] >> m_triangles[i][1] >> m_triangles[i][2];
        // cout << "Triangle: " << m_triangles[i][0] << "," << m_triangles[i][1] << "," << m_triangles[i][2];
        if (!(in.peek() == '\n' || in.peek() == '\r' || in.eof())) {
            string restOfLine;
            getline(in, restOfLine);
            // cout << "and some things";
        }
        // cout << endl;
    }
    in.close();

    centerAndScaleToUnit();
    recomputePerVertexNormals();
    recomputePerVertexTextureCoordinates();
}

void Mesh::setSingleTriangle() {
    m_vertices = {
        glm::vec3(0., 0., 0.),
        glm::vec3(1., 0., 0.),
        glm::vec3(0., 1., 0.),
        glm::vec3(1., 1., 0.),
    };
    m_triangles = {
        glm::uvec3(0, 1, 2),
        glm::uvec3(2, 1, 3),
    };
    recomputePerVertexNormals();
    recomputePerVertexTextureCoordinates();
}

void Mesh::setSimpleGrid(const glm::uvec2& _resolution) {
    m_vertices.resize(_resolution[0] * _resolution[1]);
    m_normals.resize(_resolution[0] * _resolution[1]);
    m_uvs.resize(_resolution[0] * _resolution[1]);
    m_triangles.resize(_resolution[0] * _resolution[1] * 2);

    glm::vec3 normal = glm::vec3(0., 1., 0.);
    for (size_t iz = 0; iz < _resolution[1]; iz++) {
        float z = float(iz) / (_resolution[1] - 1);
        for (size_t ix = 0; ix < _resolution[0]; ix++) {
            float x = float(ix) / (_resolution[0] - 1);
            glm::vec3 vertex = glm::vec3(x, 0., z);
            glm::vec2 uv = glm::vec2(x, z);

            size_t v0 = iz * _resolution[0] + ix;
            m_vertices[v0] = vertex;
            m_normals[v0] = normal;
            m_uvs[v0] = uv;

            if (ix == (_resolution[0] - 1) || iz == (_resolution[1] - 1))
                continue;

            size_t v1 = iz * _resolution[0] + (ix + 1);
            size_t v2 = (iz + 1) * _resolution[0] + ix;
            size_t v3 = (iz + 1) * _resolution[0] + (ix + 1);
            glm::uvec3 triangle1 = glm::uvec3(v0, v2, v1);
            glm::uvec3 triangle2 = glm::uvec3(v1, v2, v3);

            m_triangles[2 * v0] = triangle1;
            m_triangles[2 * v0 + 1] = triangle2;
        }
    }
}

void Mesh::setSimpleTerrain(const glm::uvec2& _resolution, glm::vec2 y_range) {
    setSimpleGrid(_resolution);
    for (size_t i = 0; i < _resolution[0] * _resolution[1]; i++) {
        float rng = float(rand()) / RAND_MAX;
        m_vertices[i].y = y_range[0] + rng * (y_range[1] - y_range[0]);
    }
    recomputePerVertexNormals();
}

void Mesh::setSimpleTerrain(const glm::uvec2& _resolution, const ImageBase &_heightmap) {
    setSimpleGrid(_resolution);
    for (size_t iz = 0; iz < _resolution[1]; iz++) {
        float z = float(iz) / (_resolution[1] - 1);
        for (size_t ix = 0; ix < _resolution[0]; ix++) {
            float x = float(ix) / (_resolution[0] - 1);
            size_t i = iz * _resolution[0] + ix;
            m_vertices[i].y = float(_heightmap.getPixel(x, z)[0]) / 255.f;
        }
    }
    recomputePerVertexNormals();
}

void Mesh::setCube(size_t _n) {
    size_t n_vertices = 6 * _n * _n;
    m_vertices.resize(n_vertices);
    m_normals.resize(n_vertices);

    size_t n_triangles = n_vertices * 2;
    m_triangles.resize(n_triangles);

    for (size_t face_depth = 0; face_depth < 2; face_depth++) {
        for (size_t face_axis = 0; face_axis < 3; face_axis++) {
            for (size_t i = 0; i < _n; i++) {
                float i_pos = float(i) / (_n - 1);
                for (size_t j = 0; j < _n; j++) {
                    float j_pos = float(j) / (_n - 1);

                    size_t v0 = j + _n * (i + _n * (face_axis + 3 * face_depth));

                    m_vertices[v0][face_axis] = face_depth;
                    m_vertices[v0][(face_axis + 1) % 3] = face_depth == 0 ? j_pos : i_pos;
                    m_vertices[v0][(face_axis + 2) % 3] = face_depth == 0 ? i_pos : j_pos;

                    m_normals[v0] = glm::vec3(0.);
                    m_normals[v0][face_axis] = face_depth == 0 ? -1. : 1.;

                    if (i == (_n - 1) || j == (_n - 1))
                        continue;

                    size_t v1 = (j + 1) + _n * (i + _n * (face_axis + 3 * face_depth));
                    size_t v2 = j + _n * ((i + 1) + _n * (face_axis + 3 * face_depth));
                    size_t v3 = (j + 1) + _n * ((i + 1) + _n * (face_axis + 3 * face_depth));
                    glm::uvec3 triangle1 = glm::uvec3(v0, v2, v1);
                    glm::uvec3 triangle2 = glm::uvec3(v1, v2, v3);
                    m_triangles[2 * v0] = triangle1;
                    m_triangles[2 * v0 + 1] = triangle2;
                }
            }
        }
    }

    recomputePerVertexTextureCoordinates();
}

void Mesh::setCubeSphere(size_t _n) {
    setCube(_n);
    size_t n_vertices = 6 * _n * _n;
    for (size_t i = 0; i < n_vertices; i++) {
        m_normals[i] = glm::normalize(m_vertices[i] - glm::vec3(0.5));
        m_vertices[i] = m_normals[i];
        glm::vec2 angles = Transformation::EuclidianToEuler(m_vertices[i]);

        float nb_meridiens = (_n - 1) * 4;
        m_uvs[i] = glm::vec2(
            ((angles.y / (2.f * M_PIf)) * nb_meridiens - 1) / nb_meridiens,
            0.5f - angles.x / M_PIf);
    }
}

void Mesh::setSphere(size_t nTheta, size_t nPhi) {
    m_vertices.resize(nTheta * nPhi);
    m_normals.resize(nTheta * nPhi);
    m_uvs.resize(nTheta * nPhi);
    m_triangles.resize(2 * (nTheta - 1) * (nPhi - 1));

    for (size_t thetaI = 0; thetaI < nTheta; ++thetaI) {
        float u = (float)(thetaI) / (float)(nTheta - 1);
        float theta = u * 2 * M_PIf;
        for (size_t phiI = 0; phiI < nPhi; ++phiI) {
            float v = (float)(phiI) / (float)(nPhi - 1);
            float phi = v * M_PIf;

            glm::vec3 coordinates = glm::vec3(sinf(phi) * sinf(theta), cosf(phi), sinf(phi) * cosf(theta));
            size_t v0 = phiI * nTheta + thetaI;
            m_vertices[v0] = coordinates;
            m_normals[v0] = coordinates;
            m_uvs[v0] = glm::vec2(u, v);

            if (thetaI == nTheta - 1 || phiI == nPhi - 1)
                continue;

            size_t t0 = 2 * (phiI * (nTheta - 1) + thetaI);
            size_t v1 = v0 + 1;
            size_t v2 = v0 + nTheta;
            size_t v3 = v2 + 1;
            m_triangles[t0] = glm::uvec3(v0, v2, v1);
            m_triangles[t0 + 1] = glm::uvec3(v1, v2, v3);
        }
    }
}

void Mesh::computeBoundingSphere(glm::vec3 &center, float &radius) const {
    center = glm::vec3(0.0);
    for (const glm::vec3 &p : m_vertices) {
        center += p;
    }
    center /= m_vertices.size();

    radius = 0.f;
    for (const glm::vec3 &p : m_vertices) {
        radius = std::max(radius, distance(center, p));
    }
}

void Mesh::recomputePerVertexNormals(bool angleBased) {
    m_normals.clear();
    m_normals.resize(m_vertices.size(), glm::vec3(0.0, 0.0, 0.0));

    for (unsigned int tIt = 0; tIt < m_triangles.size(); ++tIt) {
        glm::uvec3 t = m_triangles[tIt];
        glm::vec3 n_t = glm::cross(m_vertices[t[1]] - m_vertices[t[0]], m_vertices[t[2]] - m_vertices[t[0]]);
        m_normals[t[0]] += n_t;
        m_normals[t[1]] += n_t;
        m_normals[t[2]] += n_t;
    }
    for (unsigned int nIt = 0; nIt < m_normals.size(); ++nIt) {
        glm::normalize(m_normals[nIt]);
    }
}

void Mesh::recomputePerVertexTextureCoordinates() {
    m_uvs.clear();
    m_uvs.resize(m_vertices.size(), glm::vec2(0.0, 0.0));

    float xMin = FLT_MAX, xMax = FLT_MIN;
    float yMin = FLT_MAX, yMax = FLT_MIN;
    for (glm::vec3 &p : m_vertices) {
        xMin = std::min(xMin, p[0]);
        xMax = std::max(xMax, p[0]);
        yMin = std::min(yMin, p[1]);
        yMax = std::max(yMax, p[1]);
    }
    for (unsigned int pIt = 0; pIt < m_uvs.size(); ++pIt) {
        m_uvs[pIt] = glm::vec2((m_vertices[pIt][0] - xMin) / (xMax - xMin), (m_vertices[pIt][1] - yMin) / (yMax - yMin));
    }
}

bool computeBarycentrics(const glm::vec3 &v0, const glm::vec3 &v1, const glm::vec3 &v2, const glm::vec3 &normal, const glm::vec3 &p, glm::vec3 &barycentrics) {
    double total_area = glm::length(normal); // this is actually the 2 times the area but it doesn't matter for the barycentric coordinates
    barycentrics.x = glm::length(glm::cross(v1 - p, v2 - p)) / total_area - 1.e-8;
    barycentrics.y = glm::length(glm::cross(p - v0, v2 - v0)) / total_area - 1.e-8;
    barycentrics.z = glm::length(glm::cross(v1 - v0, p - v0)) / total_area - 1.e-8;
    if (barycentrics.x < 0. || 1. < barycentrics.x ||
        barycentrics.y < 0. || 1. < barycentrics.y ||
        barycentrics.z < 0. || 1. < barycentrics.z ||
        barycentrics.x + barycentrics.y + barycentrics.z < 0. || 1. < barycentrics.x + barycentrics.y + barycentrics.z) {
        return false;
    }

    return true;
}

glm::vec3 Mesh::computeheight(const glm::uvec2& _grid_resolution, float _x, float _z) const {
    // On part du principe que le terrain est une "grille" régulière et va de (0,0,0) à (1,0,1)

    uint iz = _z * (_grid_resolution[1] - 1);
    uint ix = _x * (_grid_resolution[0] - 1);
    for(uint j = 0; j < 2; j++) { 
        uint i = 2 * (iz * _grid_resolution[0] + ix) + j;

        if (i > m_triangles.size()-1)
            return glm::vec3(0.f);
        
        glm::vec3 v0 = m_vertices[m_triangles[i][0]];
        glm::vec3 v1 = m_vertices[m_triangles[i][1]];
        glm::vec3 v2 = m_vertices[m_triangles[i][2]];
        v0.y = 0.;
        v1.y = 0.;
        v2.y = 0.;
        
        glm::vec3 normal = glm::cross(v1-v0, v2-v0);
        glm::vec3 barycentrics;
        if (computeBarycentrics(v0, v1, v2, normal, glm::vec3(_x, 0.f, _z), barycentrics)) {
            return barycentrics[0]* m_vertices[m_triangles[i][0]] + barycentrics[1]* m_vertices[m_triangles[i][1]] + barycentrics[2]* m_vertices[m_triangles[i][2]];
        }
    }

    return glm::vec3(0.f);
}

glm::vec3 Mesh::computeheight(const glm::uvec2& _grid_resolution, const glm::mat4& _transfo, const glm::vec3& _p) const {
    glm::mat4 inverse = glm::inverse(_transfo);
    glm::vec4 p_terrain = inverse * glm::vec4(_p, 1.f);

    glm::vec3 point_on_terrain = computeheight(_grid_resolution, p_terrain.x/p_terrain.w, p_terrain.z/p_terrain.w);
    glm::vec4 point = _transfo *glm::vec4(point_on_terrain, 1.f);

    return glm::vec3(point.x, point.y, point.z)/point.w;
}

void Mesh::initShaderData() {
    glGenVertexArrays(1, &m_VAO);
    glBindVertexArray(m_VAO);

    glGenBuffers(1, &m_vertices_VBO);
    glBindBuffer(GL_ARRAY_BUFFER, m_vertices_VBO);
    glBufferData(GL_ARRAY_BUFFER, m_vertices.size() * sizeof(glm::vec3), m_vertices.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, m_vertices_VBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glGenBuffers(1, &m_normals_VBO);
    glBindBuffer(GL_ARRAY_BUFFER, m_normals_VBO);
    glBufferData(GL_ARRAY_BUFFER, m_normals.size() * sizeof(glm::vec3), m_normals.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, m_normals_VBO);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glGenBuffers(1, &m_uvs_VBO);
    glBindBuffer(GL_ARRAY_BUFFER, m_uvs_VBO);
    glBufferData(GL_ARRAY_BUFFER, m_uvs.size() * sizeof(glm::vec2), m_uvs.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(2);
    glBindBuffer(GL_ARRAY_BUFFER, m_uvs_VBO);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);

    glGenBuffers(1, &m_triangles_EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_triangles_EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_triangles.size() * sizeof(glm::uvec3), m_triangles.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindVertexArray(0);
}

void Mesh::render() const {
    glBindVertexArray(m_VAO); // Activate the VAO storing geometry data
    glDrawElements(GL_TRIANGLES, m_triangles.size() * 3, GL_UNSIGNED_INT, 0);
}

void Mesh::clear() {
    m_vertices.clear();
    m_normals.clear();
    m_uvs.clear();
    m_triangles.clear();
    if (m_VAO) {
        glDeleteVertexArrays(1, &m_VAO);
        m_VAO = 0;
    }
    if (m_vertices_VBO) {
        glDeleteBuffers(1, &m_vertices_VBO);
        m_vertices_VBO = 0;
    }
    if (m_normals_VBO) {
        glDeleteBuffers(1, &m_normals_VBO);
        m_normals_VBO = 0;
    }
    if (m_uvs_VBO) {
        glDeleteBuffers(1, &m_uvs_VBO);
        m_uvs_VBO = 0;
    }
    if (m_triangles_EBO) {
        glDeleteBuffers(1, &m_triangles_EBO);
        m_triangles_EBO = 0;
    }
}
