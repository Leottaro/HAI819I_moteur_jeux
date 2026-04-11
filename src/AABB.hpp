#pragma once

// Include GLEW
#include <GL/glew.h>

// GLM
#include <glm/ext.hpp>
#include <glm/glm.hpp>

// USUAL INCLUDES
#include <limits>

template <typename T>
struct AABB {
    using vec3 = glm::vec<3, T, glm::packed_highp>;

    vec3 min;
    vec3 max;

    AABB() : min(std::numeric_limits<T>::max()), max(std::numeric_limits<T>::min()) {}
    AABB(vec3 const &_min, vec3 const &_max) : min(_min), max(_max) {}

    inline void addPosition(vec3 const &v) {
        min.x = std::min(min.x, v.x);
        min.y = std::min(min.y, v.y);
        min.z = std::min(min.z, v.z);
        max.x = std::max(max.x, v.x);
        max.y = std::max(max.y, v.y);
        max.z = std::max(max.z, v.z);
    }

    inline bool isInside(vec3 const &v) const {
        return !(v.x < min.x || v.y < min.y || v.z < min.z ||
                 v.x > max.x || v.y > max.y || v.z > max.z);
    }

    bool intersect(const vec3 &origin, const vec3 &direction, T &tmin, T &tmax) const {
        // https://www.rose-hulman.edu/class/cs/csse451/AABB/#:~:text=Axis%2DAligned%20Bounding%20Boxes%20(AABBs,bound%20and%20a%20maximum%20bound.
        vec3 delta_min = min - origin;
        vec3 delta_max = max - origin;

        tmin = delta_min.x / direction.x;
        tmax = delta_max.x / direction.x;
        if (tmin > tmax)
            std::swap(tmin, tmax);

        T tmin_tmp = delta_min.y / direction.y;
        T tmax_tmp = delta_max.y / direction.y;
        if (tmin_tmp > tmax_tmp)
            std::swap(tmin_tmp, tmax_tmp);

        if (tmax_tmp < tmin || tmin_tmp > tmax)
            return false;
        tmin = std::max(tmin, tmin_tmp);
        tmax = std::min(tmax, tmax_tmp);

        tmin_tmp = delta_min.z / direction.z;
        tmax_tmp = delta_max.z / direction.z;
        if (tmin_tmp > tmax_tmp)
            std::swap(tmin_tmp, tmax_tmp);

        if (tmax_tmp < tmin || tmin_tmp > tmax)
            return false;
        tmin = std::max(tmin, tmin_tmp);
        tmax = std::min(tmax, tmax_tmp);

        return true;
    }

private:
    GLuint m_VAO = 0;
    GLuint m_vertices_VBO = 0;
    GLuint m_lines_EBO = 0;

public:
    void initShaderData() {
        glGenVertexArrays(1, &m_VAO);
        glBindVertexArray(m_VAO);

        std::vector<glm::vec3> m_vertices{
            glm::vec3(min.x, min.y, min.z),
            glm::vec3(min.x, min.y, max.z),
            glm::vec3(min.x, max.y, min.z),
            glm::vec3(min.x, max.y, max.z),
            glm::vec3(max.x, min.y, min.z),
            glm::vec3(max.x, min.y, max.z),
            glm::vec3(max.x, max.y, min.z),
            glm::vec3(max.x, max.y, max.z)};

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

    void render() const {
        glBindVertexArray(m_VAO);
        glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, 0);
    }

    void clearShaderData() {
        if (m_VAO) {
            glDeleteVertexArrays(1, &m_VAO);
            m_VAO = 0;
        }
        if (m_vertices_VBO) {
            glDeleteBuffers(1, &m_vertices_VBO);
            m_vertices_VBO = 0;
        }
        if (m_lines_EBO) {
            glDeleteBuffers(1, &m_lines_EBO);
            m_lines_EBO = 0;
        }
    }
};