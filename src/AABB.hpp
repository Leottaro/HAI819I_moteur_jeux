#pragma once

// Include GLEW
#include <GL/glew.h>

// GLM
#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

// USUAL INCLUDESs
#include <iostream>

template <typename T>
struct AABB {
    using vec3 = glm::vec<3, T, glm::packed_highp>;

    vec3 min;
    vec3 max;

    AABB() : min(std::numeric_limits<T>::max()), max(-std::numeric_limits<T>::max()) {}
    AABB(vec3 const &_min, vec3 const &_max) : min(_min), max(_max) {}

    friend std::ostream &operator<<(std::ostream &os, const AABB &aabb) {
        os << "AABB{min: " << aabb.min.x << ", " << aabb.min.y << ", " << aabb.min.z
           << " | max: " << aabb.max.x << ", " << aabb.max.y << ", " << aabb.max.z << "}";
        return os;
    }
    friend AABB operator+(const AABB &_a, const vec3 &_offset) {
        return AABB(_a.min + _offset, _a.max + _offset);
    }
    friend AABB operator+(const vec3 &_offset, const AABB &_a) {
        return AABB(_offset + _a.min, _offset + _a.max);
    }

    inline void addPosition(vec3 const &v) {
        min.x = std::min(min.x, v.x);
        min.y = std::min(min.y, v.y);
        min.z = std::min(min.z, v.z);
        max.x = std::max(max.x, v.x);
        max.y = std::max(max.y, v.y);
        max.z = std::max(max.z, v.z);
    }

    inline vec3 getCorner(int idx) const {
        return vec3(
            (idx & 1) ? max.x : min.x,
            (idx & 2) ? max.y : min.y,
            (idx & 4) ? max.z : min.z);
    }

    template <typename Func>
    inline void forAllCorners(Func &&_func) const {
        for (int i = 0; i < 8; ++i) {
            _func(getCorner(i));
        }
    }

    inline bool isInside(vec3 const &v) const {
        return !(v.x < min.x || v.y < min.y || v.z < min.z ||
                 v.x > max.x || v.y > max.y || v.z > max.z);
    }

    bool intersectRay(const vec3 &origin, const vec3 &direction, T &tmin, T &tmax) const {
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

    // Return if there is an intersection and the minimal vector "dist" to move "_other" so it doesn't intersect
    bool intersectAABB(const AABB &_other, vec3 &dist) const {
        const T overlapX = std::min(max.x, _other.max.x) - std::max(min.x, _other.min.x);
        const T overlapY = std::min(max.y, _other.max.y) - std::max(min.y, _other.min.y);
        const T overlapZ = std::min(max.z, _other.max.z) - std::max(min.z, _other.min.z);

        if (overlapX < std::numeric_limits<T>::min() || overlapY < std::numeric_limits<T>::min() || overlapZ < std::numeric_limits<T>::min()) {
            dist = vec3(T(0));
            return false;
        }

        const vec3 center = (min + max) * T(0.5);
        const vec3 other_center = (_other.min + _other.max) * T(0.5);

        dist = vec3(T(0));
        bool changed = false;
        if (overlapX <= overlapY && overlapX <= overlapZ) {
            dist.x = (other_center.x < center.x) ? -overlapX : overlapX;
            changed = true;
        } else if (overlapY <= overlapZ) {
            dist.y = (other_center.y < center.y) ? -overlapY : overlapY;
            changed = true;
        } else {
            dist.z = (other_center.z < center.z) ? -overlapZ : overlapZ;
            changed = true;
        }
        return changed;
    }

    // Returns true if _other moving by _other_vel will intersect *this.
    // Sets t to the earliest time in [0,1] at which (_other + vel*t) first touches *this.
    // Sets normal to the collision surface normal (points from *this toward _other).
    bool intersectAABB(const AABB &_other, const vec3 &_other_vel, T &t, vec3 &normal) const {
        // https://emanueleferonato.com/2021/10/21/understanding-physics-continuous-collision-detection-using-swept-aabb-method-and-minkowski-sum/
        vec3 other_center = 0.5f * (_other.min + _other.max);
        vec3 other_half = 0.5f * (_other.max - _other.min);
        AABB minkowski(min - other_half, max + other_half);
        std::cout << std::endl
                  << "*this : " << *this << std::endl
                  << "other : " << _other << std::endl
                  << "vel : " << glm::to_string(_other_vel) << std::endl;

        T ttemp;
        bool intersect = minkowski.intersectRay(other_center, _other_vel, t, ttemp);
        std::cout << "intersect=" << intersect << "\tt=" << t << "\tttemp=" << ttemp << std::endl;
        if (intersect && t >= 0.f && t <= 1.f) {
            vec3 dist;
            intersectAABB(_other + (t + 0.01f) * _other_vel, dist);

            normal.x = dist.x < T(0)   ? T(-1)
                       : dist.x > T(0) ? T(1)
                                       : T(0);
            normal.y = dist.y < T(0)   ? T(-1)
                       : dist.y > T(0) ? T(1)
                                       : T(0);
            normal.z = dist.z < T(0)   ? T(-1)
                       : dist.z > T(0) ? T(1)
                                       : T(0);
            normal = glm::normalize(normal);
            std::cout << "normal: " << glm::to_string(normal) << std::endl;

            return true;
        }

        return false;
    }

private:
    GLuint m_VAO = 0;
    GLuint m_vertices_VBO = 0;
    GLuint m_lines_EBO = 0;

public:
    ~AABB() { clearShaderData(); }
    void initShaderData() {
        glGenVertexArrays(1, &m_VAO);
        glBindVertexArray(m_VAO);

        glGenBuffers(1, &m_vertices_VBO);
        glBindBuffer(GL_ARRAY_BUFFER, m_vertices_VBO);
        glEnableVertexAttribArray(0);
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
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_lines.size() * sizeof(glm::uvec2), m_lines.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        updateShaderData();
    }

    void updateShaderData() {
        std::vector<glm::vec3> m_vertices;
        m_vertices.reserve(8);
        forAllCorners([&m_vertices](const vec3 &corner) {
            m_vertices.push_back(glm::vec3(corner.x, corner.y, corner.z));
        });

        glBindVertexArray(m_VAO);
        glBindBuffer(GL_ARRAY_BUFFER, m_vertices_VBO);
        glBufferData(GL_ARRAY_BUFFER, m_vertices.size() * sizeof(glm::vec3), m_vertices.data(), GL_STATIC_DRAW);
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