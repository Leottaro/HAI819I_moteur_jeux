#pragma once

// GLM
#include <glm/ext.hpp>
#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>
#include <glm/gtx/string_cast.hpp>

// USUAL INCLUDES
#include <math.h>
#include <set>
#include <map>
#include <bitset>
#include <memory>

// -------------------------------------------------------------------------
// MATH HELPERS
// -------------------------------------------------------------------------

namespace MathHelpers {
template <typename T>
using vec2 = glm::vec<2, T, glm::highp>;
template <typename T>
using vec3 = glm::vec<3, T, glm::highp>;
template <typename T>
using vec4 = glm::vec<4, T, glm::highp>;
template <typename T>
using mat4 = glm::mat<4, 4, T, glm::highp>;

template <typename T>
using pvec2 = glm::vec<2, T, glm::packed_highp>;
using fpvec2 = glm::vec<2, float, glm::packed_highp>;
using dpvec2 = glm::vec<2, double, glm::packed_highp>;
using upvec2 = glm::vec<2, uint32_t, glm::packed_highp>;
using u8pvec2 = glm::vec<2, uint8_t, glm::packed_highp>;
using i8pvec2 = glm::vec<2, int8_t, glm::packed_highp>;
template <typename T>
using pvec3 = glm::vec<3, T, glm::packed_highp>;
using fpvec3 = glm::vec<3, float, glm::packed_highp>;
using dpvec3 = glm::vec<3, double, glm::packed_highp>;
using upvec3 = glm::vec<3, uint32_t, glm::packed_highp>;
using u8pvec3 = glm::vec<3, uint8_t, glm::packed_highp>;
using i8pvec3 = glm::vec<3, int8_t, glm::packed_highp>;
template <typename T>
using pvec4 = glm::vec<4, T, glm::packed_highp>;
using fpvec4 = glm::vec<4, float, glm::packed_highp>;
using dpvec4 = glm::vec<4, double, glm::packed_highp>;
using upvec4 = glm::vec<4, uint32_t, glm::packed_highp>;
using u8pvec4 = glm::vec<4, uint8_t, glm::packed_highp>;
using i8pvec4 = glm::vec<4, int8_t, glm::packed_highp>;
template <typename T>
using pmat4 = glm::mat<4, 4, T, glm::packed_highp>;
using fpmat4 = glm::mat<4, 4, float, glm::packed_highp>;
using dpmat4 = glm::mat<4, 4, double, glm::packed_highp>;
using upmat4 = glm::mat<4, 4, uint32_t, glm::packed_highp>;
using u8pmat4 = glm::mat<4, 4, uint8_t, glm::packed_highp>;
using i8pmat4 = glm::mat<4, 4, int8_t, glm::packed_highp>;

template <typename T>
constexpr T EPSILON();
template <>
constexpr float EPSILON<float>() {
    return 1.e-4f;
}
template <>
constexpr double EPSILON<double>() {
    return 1.e-8;
}

constexpr float M_PI_SAFE{M_PIf - 1.e-3f};
constexpr float M_PI_2_SAFE{M_PI_2f - 1.e-3f};
constexpr float M_PI_4_SAFE{M_PI_4f - 1.e-3f};
constexpr glm::vec3 VEC_ZERO{0.f, 0.f, 0.f};
constexpr glm::vec3 VEC_RIGHT{1.f, 0.f, 0.f};
constexpr glm::vec3 VEC_UP{0.f, 1.f, 0.f};
constexpr glm::vec3 VEC_FRONT{0.f, 0.f, 1.f};

template <typename T>
constexpr vec3<T> applyTransformation(const vec3<T>& vec, float w, const mat4<T>& transfo) {
    vec4<T> temp = transfo * vec4<T>(vec.x, vec.y, vec.z, w);
    return temp.w == 0. ? vec3<T>(temp.x, temp.y, temp.z) : vec3<T>(temp.x, temp.y, temp.z) / temp.w;
}
template <typename T>
constexpr vec3<T> projectVectorOnPlane(const vec3<T>& _vec, const vec3<T>& _normal) {
    return glm::cross(glm::normalize(_normal), glm::cross(_vec, glm::normalize(_normal)));
}
template <typename T>
constexpr vec3<T> projectPointOnPlane(const vec3<T>& _point, const vec3<T>& _origin, const vec3<T>& _normal) {
    return _origin + projectVectorOnPlane(_point - _origin, _normal);
}
template <typename T>
constexpr vec3<T> projectVectorOnLine(const vec3<T>& _vec, const vec3<T>& _direction) {
    return glm::dot(_vec, _direction) * _direction;
}
template <typename T>
constexpr vec3<T> projectPointOnLine(const vec3<T>& _point, const vec3<T>& _origin, const vec3<T>& _direction) {
    return _origin + projectVectorOnLine(_point - _origin, _direction);
}

template <typename T>
constexpr bool computeBarycentrics(const vec3<T>& v0, const vec3<T>& v1, const vec3<T>& v2, const vec3<T>& normal, const vec3<T>& p, vec3<T>& barycentrics) {
    double total_area_sq = glm::length2(normal);
    if (total_area_sq < EPSILON<T>())
        return false;

    // Signed barycentric coordinates
    barycentrics.x = glm::dot(glm::cross(v1 - p, v2 - p), normal) / total_area_sq;
    barycentrics.y = glm::dot(glm::cross(v2 - p, v0 - p), normal) / total_area_sq;
    barycentrics.z = glm::dot(glm::cross(v0 - p, v1 - p), normal) / total_area_sq;

    if (barycentrics.x < -EPSILON<T>() || T(1. + EPSILON<T>()) < barycentrics.x ||
        barycentrics.y < -EPSILON<T>() || T(1. + EPSILON<T>()) < barycentrics.y ||
        barycentrics.z < -EPSILON<T>() || T(1. + EPSILON<T>()) < barycentrics.z) {
        return false;
    }

    return true;
}

template <typename T>
constexpr bool rayTriangleIntersection(const vec3<T>& origin, const vec3<T>& direction,
                                       const vec3<T>& v0, const vec3<T>& v1, const vec3<T>& v2, const vec3<T>& normal,
                                       T& t, vec3<T>& intersection, vec3<T>& barycentrics) {
    // Check if ray is parallel
    double dot = glm::dot(direction, normal);
    if (std::abs(dot) <= EPSILON<T>()) {
        return false;
    }

    // determine intersection
    t = -(glm::dot(normal, origin - v0)) / dot;
    intersection = origin + t * direction;

    // barycentric coordinates
    return computeBarycentrics(v0, v1, v2, normal, intersection, barycentrics);
}

template <typename T, size_t n>
struct glmVecLexicoGraphic {
    bool operator()(const glm::vec<n, T, glm::highp>& a, const glm::vec<n, T, glm::highp>& b) const {
        if constexpr (n >= 1)
            if (a.x != b.x)
                return a.x < b.x;
        if constexpr (n >= 2)
            if (a.y != b.y)
                return a.y < b.y;
        if constexpr (n >= 3)
            if (a.z != b.z)
                return a.z < b.z;
        if constexpr (n >= 4)
            if (a.w != b.w)
                return a.w < b.w;
        return false;
    }
};

template <typename T, size_t n>
using VecSet = std::set<glm::vec<n, T, glm::highp>, glmVecLexicoGraphic<T, n>>;
template <typename T, size_t n, typename V>
using VecMap = std::map<glm::vec<n, T, glm::highp>, V, glmVecLexicoGraphic<T, n>>;
} // namespace MathHelpers

// -------------------------------------------------------------------------
// ECS HELPERS
// -------------------------------------------------------------------------

namespace ECS_HELPERS {
// Helper that checks if the type T is in the Tuple
template <typename Tuple, typename T, std::size_t I = 0>
constexpr bool is_type_in_tuple() {
    if constexpr (I >= std::tuple_size_v<Tuple>)
        return false;
    else if constexpr (std::is_same_v<T, std::tuple_element_t<I, Tuple>>)
        return true;
    else
        return is_type_in_tuple<Tuple, T, I + 1>();
}

// Helper that finds the index of the type T in the Tuple
template <typename Tuple, typename T, std::size_t I = 0>
constexpr std::size_t type_index() {
    static_assert(I < std::tuple_size_v<Tuple>, "Component not in list");
    if constexpr (std::is_same_v<T, std::tuple_element_t<I, Tuple>>)
        return I;
    else
        return type_index<Tuple, T, I + 1>();
}

// Helper that executes the function F for each type in the Tuple
template <typename Tuple, typename F, std::size_t... Is>
constexpr void for_each_type_impl(F&& f, std::index_sequence<Is...>) {
    (f.template operator()<std::tuple_element_t<Is, Tuple>>(), ...);
}

// Helper that checks if the Tuple1 is a superset of the Tuple2
template <typename Tuple1, typename Tuple2>
constexpr bool is_superset() {
    bool res = true;
    for_each_type_impl<Tuple2>([&]<typename T>() {
        if (!is_type_in_tuple<Tuple1, T>())
            res = false;
    },
                               std::make_index_sequence<std::tuple_size_v<Tuple2>>{});
    return res;
}

template <typename Tuple, typename SubTuple, std::size_t... Is>
constexpr std::bitset<std::tuple_size_v<Tuple>> make_signature_impl(std::index_sequence<Is...>) {
    constexpr std::size_t N = std::tuple_size_v<Tuple>;
    char str[N + 1];
    for (std::size_t i = 0; i < N; ++i)
        str[i] = '0';
    str[N] = '\0';
    ((str[N - 1 - type_index<Tuple, std::tuple_element_t<Is, SubTuple>>()] = '1'), ...);
    return std::bitset<N>(str);
}

template <typename Tuple, typename SubTuple>
constexpr std::bitset<std::tuple_size_v<Tuple>> make_signature() {
    return make_signature_impl<Tuple, SubTuple>(
        std::make_index_sequence<std::tuple_size_v<SubTuple>>{});
}
} // namespace ECS_HELPERS

// -------------------------------------------------------------------------
// STORAGE HELPERS
// -------------------------------------------------------------------------

template <typename T, size_t BATCH_BYTES, typename Key, typename KeyLess = std::less<Key>>
class ContiguousStorage {
    static_assert(sizeof(T) <= BATCH_BYTES);                      // We have to have the room for at least one chunk
    static_assert(alignof(T) <= alignof(std::max_align_t));       // If this fails, we need aligned_alloc
    static constexpr size_t BATCH_SIZE = BATCH_BYTES / sizeof(T); // nombre de T qui font au max BATCH_BYTES

    std::vector<std::unique_ptr<std::array<T, BATCH_SIZE>>> m_storage{};  // Ensemble de batch de T
    std::vector<std::unique_ptr<std::array<bool, BATCH_SIZE>>> m_alive{}; // Ensemble de batch de "en vie ?"
    std::map<Key, glm::uvec2, KeyLess> m_lookup_table{};                  // Key -> idx de batch et idx de T

    std::vector<glm::uvec2> m_free_list{};
    size_t nb_elements{0};

public:
    ContiguousStorage(ContiguousStorage&&) = delete;
    ContiguousStorage& operator=(ContiguousStorage&&) = delete;
    ContiguousStorage(const ContiguousStorage& other) = delete;
    ContiguousStorage& operator=(const ContiguousStorage&) = delete;
    ~ContiguousStorage() { clear(); }

    ContiguousStorage() {}

    inline size_t size() const { return nb_elements; }
    inline T* at(const Key& _key) {
        auto it = m_lookup_table.find(_key);
        if (it == m_lookup_table.end())
            return nullptr;
        return &m_storage[it->second.x]->at(it->second.y);
    }
    inline const T* at(const Key& _key) const {
        auto it = m_lookup_table.find(_key);
        if (it == m_lookup_table.end())
            return nullptr;
        return &m_storage[it->second.x]->at(it->second.y);
    }
    inline T& operator[](const Key& _key) {
        glm::uvec2 pos = m_lookup_table[_key];
        return m_storage[pos.x]->at(pos.y);
    }
    inline const T& operator[](const Key& _key) const {
        glm::uvec2 pos = m_lookup_table[_key];
        return m_storage[pos.x]->at(pos.y);
    }

    template <typename... Args>
    T& emplace(Key _key, Args&&... args) {
        auto it = m_lookup_table.find(_key);
        if (it != m_lookup_table.end()) {
            return m_storage[it->second.x]->at(it->second.y);
        }

        glm::uvec2 indices;
        if (!m_free_list.empty()) {
            indices = m_free_list.back();
            m_free_list.pop_back();
        } else {
            indices.x = nb_elements / BATCH_SIZE;
            indices.y = nb_elements - BATCH_SIZE * indices.x;
            if (indices.y == 0) {
                m_storage.push_back(std::make_unique<std::array<T, BATCH_SIZE>>());
                m_alive.push_back(std::make_unique<std::array<bool, BATCH_SIZE>>());
            }
        }

        // Construct directly in place — no move, no copy
        T* slot = &m_storage[indices.x]->at(indices.y);
        std::construct_at(slot, args...);

        m_alive[indices.x]->at(indices.y) = true;
        m_lookup_table.insert({_key, indices});
        nb_elements += 1;
        return *slot;
    }

    bool remove(const Key& _chunk_pos) {
        glm::uvec2 indices = m_lookup_table[_chunk_pos];
        if (indices.x >= m_alive.size() || !m_alive[indices.x]->at(indices.y))
            return false;
        // Destruct directly in place
        T* slot = &m_storage[indices.x]->at(indices.y);
        std::destroy_at(slot);
        nb_elements -= 1;
        m_alive[indices.x]->at(indices.y) = false;
        m_free_list.emplace_back(indices);
        m_lookup_table.erase(_chunk_pos);
        return true;
    }

    inline void forEach(std::function<void(T&)> fn) {
        for (uint batch_i = 0; batch_i < m_storage.size(); batch_i++)
            for (uint chunk_i = 0; chunk_i < BATCH_SIZE; chunk_i++)
                if (m_alive[batch_i]->at(chunk_i))
                    fn(m_storage[batch_i]->at(chunk_i));
    }
    inline void forEach(std::function<void(const T&)> fn) const {
        for (uint batch_i = 0; batch_i < m_storage.size(); batch_i++)
            for (uint chunk_i = 0; chunk_i < BATCH_SIZE; chunk_i++)
                if (m_alive[batch_i]->at(chunk_i))
                    fn(m_storage[batch_i]->at(chunk_i));
    }

    inline void clear() {
        for (uint batch_i = 0; batch_i < m_storage.size(); batch_i++)
            for (uint chunk_i = 0; chunk_i < BATCH_SIZE; chunk_i++)
                if (m_alive[batch_i]->at(chunk_i))
                    std::destroy_at(&m_storage[batch_i]->at(chunk_i));
        m_storage.clear();
        m_alive.clear();
        m_lookup_table.clear();
        m_free_list.clear();
        nb_elements = 0;
    }
};