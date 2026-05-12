#pragma once

// Include GLEW
#include <GL/glew.h>

// GLM
#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

// USUAL INCLUDESs
#include "Block.hpp"
#include "Helpers.hpp"
#include <iostream>

template <typename T>
struct AABB {
    using vec3 = glm::vec<3, T, glm::defaultp>;
    static constexpr T POSITIVE_EPSILON = std::numeric_limits<T>::min();
    static constexpr T NEGATIVE_EPSILON = -std::numeric_limits<T>::min();
    static constexpr T POSITIVE_MAX = std::numeric_limits<T>::max();
    static constexpr T NEGATIVE_MAX = -std::numeric_limits<T>::max();

    vec3 min;
    vec3 max;

    AABB() : min(POSITIVE_MAX), max(NEGATIVE_MAX) {}
    AABB(const vec3& _min, const vec3& _max) : min(_min), max(_max) {}

    friend std::ostream& operator<<(std::ostream& os, const AABB& aabb) {
        os << "AABB{min: " << aabb.min.x << ", " << aabb.min.y << ", " << aabb.min.z
           << " | max: " << aabb.max.x << ", " << aabb.max.y << ", " << aabb.max.z << "}";
        return os;
    }
    friend AABB operator+(const AABB& _a, const vec3& _offset) {
        return AABB(_a.min + _offset, _a.max + _offset);
    }
    friend AABB operator+(const vec3& _offset, const AABB& _a) {
        return AABB(_offset + _a.min, _offset + _a.max);
    }

    inline void addPosition(const vec3& v) {
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
    inline void forAllCorners(Func&& _func) const {
        for (int i = 0; i < 8; ++i) {
            _func(getCorner(i));
        }
    }

    inline bool isInside(const vec3& v) const {
        return !(v.x < min.x || v.y < min.y || v.z < min.z ||
                 v.x > max.x || v.y > max.y || v.z > max.z);
    }

    // Face indices: -Z=0, -X=1, -Y=2, +Z=3, +X=4, +Y=5
    inline bool intersectRay(const vec3& origin, const vec3& direction, T& tmin, T& tmax, uint& face_min, uint& face_max) const {
        vec3 delta_min = min - origin;
        vec3 delta_max = max - origin;

        T t0, t1;
        int f0, f1;

        // X slab: -X=1, +X=4
        t0 = delta_min.x / direction.x;
        t1 = delta_max.x / direction.x;
        f0 = 1;
        f1 = 4; // -X, +X
        if (t0 > t1) {
            std::swap(t0, t1);
            std::swap(f0, f1);
        }
        tmin = t0;
        tmax = t1;
        face_min = f0;
        face_max = f1;

        // Y slab: -Y=2, +Y=5
        T tmin_tmp = delta_min.y / direction.y;
        T tmax_tmp = delta_max.y / direction.y;
        int fmin_tmp = 2, fmax_tmp = 5; // -Y, +Y
        if (tmin_tmp > tmax_tmp) {
            std::swap(tmin_tmp, tmax_tmp);
            std::swap(fmin_tmp, fmax_tmp);
        }

        if (tmax_tmp < tmin || tmin_tmp > tmax)
            return false;
        if (tmin_tmp > tmin) {
            tmin = tmin_tmp;
            face_min = fmin_tmp;
        }
        if (tmax_tmp < tmax) {
            tmax = tmax_tmp;
            face_max = fmax_tmp;
        }

        // Z slab: -Z=0, +Z=3
        tmin_tmp = delta_min.z / direction.z;
        tmax_tmp = delta_max.z / direction.z;
        fmin_tmp = 0;
        fmax_tmp = 3; // -Z, +Z
        if (tmin_tmp > tmax_tmp) {
            std::swap(tmin_tmp, tmax_tmp);
            std::swap(fmin_tmp, fmax_tmp);
        }

        if (tmax_tmp < tmin || tmin_tmp > tmax)
            return false;
        if (tmin_tmp > tmin) {
            tmin = tmin_tmp;
            face_min = fmin_tmp;
        }
        if (tmax_tmp < tmax) {
            tmax = tmax_tmp;
            face_max = fmax_tmp;
        }

        return true;
    }

    // Return if there is an intersection and the minimal vector "dist" to move "_other" so it doesn't intersect
    inline bool intersectAABB(const AABB& _other, vec3& dist) const {
        const T overlapX = std::min(max.x, _other.max.x) - std::max(min.x, _other.min.x);
        const T overlapY = std::min(max.y, _other.max.y) - std::max(min.y, _other.min.y);
        const T overlapZ = std::min(max.z, _other.max.z) - std::max(min.z, _other.min.z);

        if (overlapX <= T(0) || overlapY <= T(0) || overlapZ <= T(0)) {
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
    inline bool intersectAABB(const AABB& _other, const vec3& _other_vel, T& t, vec3& normal) const {
        // https://emanueleferonato.com/2021/10/21/understanding-physics-continuous-collision-detection-using-swept-aabb-method-and-minkowski-sum/
        vec3 other_center = 0.5f * (_other.min + _other.max);
        vec3 other_half = 0.5f * (_other.max - _other.min);
        AABB minkowski(min - other_half, max + other_half);
        // std::cout << std::endl
        //           << "*this : " << *this << std::endl
        //           << "other : " << _other << std::endl
        //           << "minkowski : " << minkowski << std::endl
        //           << "vel : " << glm::to_string(_other_vel) << std::endl;

        T ttemp;
        uint face_min, face_max;
        bool intersect = minkowski.intersectRay(other_center, _other_vel, t, ttemp, face_min, face_max);
        // std::cout << "intersect=" << intersect << "\tt=" << t << "\tttemp=" << ttemp << std::endl;
        if (intersect && t >= T(0) && t <= T(1)) {
            normal = Block::FACE_DATA[face_min].normal;
            return true;
        }

        return false;
    }
};

struct AABBRenderer {
    static constexpr std::array<std::array<uint32_t, 2>, 12> LINES{{// Z
                                                                    {0, 1},
                                                                    {2, 3},
                                                                    {4, 5},
                                                                    {6, 7},

                                                                    // Y
                                                                    {0, 2},
                                                                    {1, 3},
                                                                    {4, 6},
                                                                    {5, 7},

                                                                    // X
                                                                    {0, 4},
                                                                    {1, 5},
                                                                    {2, 6},
                                                                    {3, 7}}};
    inline static GLuint LINES_EBO{0};

    GLuint m_VAO{0};
    GLuint m_vertices_VBO{0};
    std::array<MathHelpers::fpvec3, 8> m_vertices;

    void resetShaderData() {
        clearShaderData();
        glGenVertexArrays(1, &m_VAO);
        glBindVertexArray(m_VAO);
        glGenBuffers(1, &m_vertices_VBO);
        glBindBuffer(GL_ARRAY_BUFFER, m_vertices_VBO);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glBufferData(GL_ARRAY_BUFFER, m_vertices.size() * sizeof(MathHelpers::fpvec3), m_vertices.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, LINES_EBO);
    }

public:
    AABBRenderer(AABBRenderer&& other) : m_VAO(other.m_VAO), m_vertices_VBO(other.m_vertices_VBO) {
        other.m_VAO = other.m_vertices_VBO = 0;
    }
    AABBRenderer& operator=(AABBRenderer&& other) {
        if (this == &other)
            return *this;
        clearShaderData();

        m_VAO = other.m_VAO;
        m_vertices_VBO = other.m_vertices_VBO;

        other.m_VAO = other.m_vertices_VBO = 0;
        return *this;
    }
    AABBRenderer(const AABBRenderer& other): m_vertices(other.m_vertices) {
        resetShaderData();
    }
    AABBRenderer& operator=(const AABBRenderer& other) {
        if (this == &other)
            return *this;
        m_vertices = other.m_vertices;
        resetShaderData();
        return *this;
    }
    ~AABBRenderer() { clearShaderData(); }

    AABBRenderer() {}
    template <typename T>
    AABBRenderer(const AABB<T>& _aabb) { initShaderData(_aabb); }

    template <typename T>
    inline void initShaderData(const AABB<T>& _aabb) {
        clearShaderData();

        if (LINES_EBO == 0) {
            glGenBuffers(1, &LINES_EBO);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, LINES_EBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, 96UL, LINES.data(), GL_STATIC_DRAW);
        }

        size_t i = 0;
        _aabb.forAllCorners([&](const auto& corner) {
            m_vertices[i].x = corner.x;
            m_vertices[i].y = corner.y;
            m_vertices[i].z = corner.z;
            i++;
        });

        glGenVertexArrays(1, &m_VAO);
        glBindVertexArray(m_VAO);

        glGenBuffers(1, &m_vertices_VBO);
        glBindBuffer(GL_ARRAY_BUFFER, m_vertices_VBO);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glBufferData(GL_ARRAY_BUFFER, m_vertices.size() * sizeof(MathHelpers::fpvec3), m_vertices.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, LINES_EBO);
    }

    inline void render() const {
        glBindVertexArray(m_VAO);
        glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, 0);
    }

    inline void clearShaderData() {
        if (m_VAO) {
            glDeleteVertexArrays(1, &m_VAO);
            m_VAO = 0;
        }
        if (m_vertices_VBO) {
            glDeleteBuffers(1, &m_vertices_VBO);
            m_vertices_VBO = 0;
        }
    }
};