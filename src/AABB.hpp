#pragma once

// Include GLEW
#include <GL/glew.h>

// GLM
#include <glm/glm.hpp>

// USUAL INCLUDESs

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

    // Return if there is an intersection and the minimal vector "dist" to move "_other" so it doesn't intersect
    bool intersect(const AABB &_other, vec3 &dist) const {
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
        if (overlapX <= overlapY && overlapX <= overlapZ) {
            dist.x = (other_center.x < center.x) ? -overlapX : overlapX;
        } else if (overlapY <= overlapZ) {
            dist.y = (other_center.y < center.y) ? -overlapY : overlapY;
        } else {
            dist.z = (other_center.z < center.z) ? -overlapZ : overlapZ;
        }

        return true;
    }

    // Returns true if _other moving by _other_vel will intersect *this.
    // Sets t to the earliest time in [0,1] at which (_other + vel*t) first touches *this.
    bool intersect(const AABB &_other, const vec3 &_other_vel, T &t) const {
        T t_enter = T(0);
        T t_exit = T(1); // clamp sweep to one step (vel is a full-frame delta)

        // For each axis compute the entry/exit times of the overlapping slab.
        for (int i = 0; i < 3; ++i) {
            const T vel = _other_vel[i];
            const T a_min = min[i], a_max = max[i];               // *this  (static)
            const T b_min = _other.min[i], b_max = _other.max[i]; // _other (moving)

            if (std::abs(vel) < std::numeric_limits<T>::epsilon()) {
                // No motion on this axis — if already separated, no collision possible.
                if (b_max < a_min || b_min > a_max)
                    return false;
                // Otherwise the slab is permanently overlapping; don't narrow [t_enter, t_exit].
            } else {
                // Time at which the leading and trailing faces meet.
                // b moves toward a, so:
                //   entry = gap between closest faces / vel
                //   exit  = gap between farthest faces / vel
                T inv_vel = T(1) / vel;
                T t0 = (a_min - b_max) * inv_vel; // _other's front face reaches *this's back face
                T t1 = (a_max - b_min) * inv_vel; // _other's back face reaches *this's front face

                if (t0 > t1)
                    std::swap(t0, t1); // ensure t0 <= t1 regardless of direction

                t_enter = std::max(t_enter, t0);
                t_exit = std::min(t_exit, t1);

                if (t_enter > t_exit)
                    return false; // slabs don't overlap in time
            }
        }

        t = t_enter;
        return true;
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
        std::vector<glm::vec3> m_vertices{
            glm::vec3(min.x, min.y, min.z),
            glm::vec3(min.x, min.y, max.z),
            glm::vec3(min.x, max.y, min.z),
            glm::vec3(min.x, max.y, max.z),
            glm::vec3(max.x, min.y, min.z),
            glm::vec3(max.x, min.y, max.z),
            glm::vec3(max.x, max.y, min.z),
            glm::vec3(max.x, max.y, max.z)};

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