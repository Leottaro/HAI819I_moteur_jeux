#pragma once
#include "AABB.hpp"

struct Frustum {
private:
    glm::vec4 m_left;
    glm::vec4 m_right;

    glm::vec4 m_bottom;
    glm::vec4 m_top;

    glm::vec4 m_near;
    glm::vec4 m_far;

    constexpr bool isVisible(const glm::vec4& _plane, float x, float y, float z) const {
        return _plane.x * x + _plane.y * y + _plane.z * z + _plane.w >= 0;
    }
    constexpr bool isVisible(const glm::vec4& _plane, const glm::vec3& _pos) const {
        return _plane.x * _pos.x + _plane.y * _pos.y + _plane.z * _pos.z + _plane.w >= 0;
    }
    constexpr bool isVisible(const glm::vec4& plane, const AABB<float>& aabb) const {
        return isVisible(plane,
                         plane.x >= 0 ? aabb.max.x : aabb.min.x,
                         plane.y >= 0 ? aabb.max.y : aabb.min.y,
                         plane.z >= 0 ? aabb.max.z : aabb.min.z);
    }

public:
    Frustum() {};

    inline void updatePlanes(const glm::mat4& view_projection_transpose) {
        m_left = view_projection_transpose[3] + view_projection_transpose[0];
        m_right = view_projection_transpose[3] - view_projection_transpose[0];
        m_bottom = view_projection_transpose[3] + view_projection_transpose[1];
        m_top = view_projection_transpose[3] - view_projection_transpose[1];
        m_near = view_projection_transpose[3] + view_projection_transpose[2];
        m_far = view_projection_transpose[3] - view_projection_transpose[2];
    }
    inline void updatePlanes(const glm::mat4& projection, const glm::mat4& view) {
        updatePlanes(glm::transpose(projection * view));
    }

    inline bool isVisible(float x, float y, float z) const {
        return !(!isVisible(m_near, x, y, z) ||
                 !isVisible(m_far, x, y, z) ||
                 !isVisible(m_top, x, y, z) ||
                 !isVisible(m_bottom, x, y, z) ||
                 !isVisible(m_right, x, y, z) ||
                 !isVisible(m_left, x, y, z));
    }
    inline bool isVisible(const glm::vec3& _pos) const {
        return !(!isVisible(m_near, _pos) ||
                 !isVisible(m_far, _pos) ||
                 !isVisible(m_top, _pos) ||
                 !isVisible(m_bottom, _pos) ||
                 !isVisible(m_right, _pos) ||
                 !isVisible(m_left, _pos));
    }

    bool isVisible(const AABB<float>& aabb) const {
        return !(!isVisible(m_near, aabb) ||
                 !isVisible(m_far, aabb) ||
                 !isVisible(m_top, aabb) ||
                 !isVisible(m_bottom, aabb) ||
                 !isVisible(m_right, aabb) ||
                 !isVisible(m_left, aabb));
    }
};