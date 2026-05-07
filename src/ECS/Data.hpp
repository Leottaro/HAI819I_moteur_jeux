#pragma once

// GLEW
#include <GL/glew.h>

// GLM
#include <glm/ext.hpp>
#include <glm/glm.hpp>
// #define GLM_ENABLE_EXPERIMENTAL
// #include <glm/gtx/string_cast.hpp>

// USUAL INCLUDES
#include "src/AABB.hpp"
#include "src/Chunk.hpp"
#include "src/Camera.hpp"
#include <concepts>
#include <array>
#include <bitset>
#include <set>
#include <queue>
#include <map>
#include <vector>

// Inspired by https://austinmorlan.com/posts/entity_component_system/

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
// ECS DATA
// -------------------------------------------------------------------------

namespace ECS {
using ComponentId = std::uint8_t;
using EntityId = std::uint16_t;
constexpr EntityId MAX_ENTITIES = 65535;

// -------------------------------------------------------------------------
// COMPONENTS
// -------------------------------------------------------------------------

struct Position {
    Chunk* current_chunk{nullptr};
    glm::vec3 pos{0.f, 0.f, 0.f};
};
struct Collision {
    std::vector<AABB<float>> hitboxes{};
};
struct Velocity {
    glm::vec3 vel{0.f, 0.f, 0.f};
};
struct Groundable {
    bool on_ground{true};
};
struct PhysicsStats {
    float weight{500.f};
    float volume{1.f};
    float drag{1.05f};
};
struct CollisionDisplay {};
struct Camerable {
    Camera* current_camera{nullptr};
};

// L'ensemble des composants, tout le reste est dérivé automatiquement
using ComponentList = std::tuple<
    Position,
    Collision,
    Velocity,
    Groundable,
    PhysicsStats,
    CollisionDisplay,
    Camerable>;

// Le nombre total de composants
constexpr std::size_t NB_COMPONENTS = std::tuple_size_v<ComponentList>;

// Un enseble de bits: le i-ème bit décrit si l'entité "possède" le composant i
// Permets de savoir quels composants a une entitée
using ComponentSignature = std::bitset<NB_COMPONENTS>;

// Permets de templater un Component en vérifiant si il est dans ComponentList
// S'uttilise dans un template a la place d'un typename
template <typename T>
concept Component = ECS_HELPERS::is_type_in_tuple<ComponentList, T>();

// Permets de templater un Tuple de component en vérifiant si ComponentList est son superset
// S'uttilise dans un template a la place d'un typename
template <typename Tuple>
concept ComponentTuple = ECS_HELPERS::is_superset<ComponentList, Tuple>();

// Permets de récuper l'indice d'un composant dans ComponentList
// S'uttilise comme ça: ECS::component_id<T>
template <Component C>
constexpr ComponentId component_id = static_cast<ComponentId>(ECS_HELPERS::type_index<ComponentList, C>());

// Permets d'éxécuter une fonction pour tout types de composants
// S'uttilise comme ça: ECS::for_each_component([&]<ECS::Component C>() { ... });
template <ComponentTuple Tuple, typename F>
constexpr void for_each_component_tuple(F&& f) {
    ECS_HELPERS::for_each_type_impl<Tuple, F>(
        std::forward<F>(f),
        std::make_index_sequence<std::tuple_size_v<Tuple>>{});
}
template <typename F>
constexpr void for_each_components(F&& f) {
    ECS_HELPERS::for_each_type_impl<ComponentList, F>(
        std::forward<F>(f),
        std::make_index_sequence<NB_COMPONENTS>{});
}

// -------------------------------------------------------------------------
// ENTITIES
// -------------------------------------------------------------------------

struct TestEntity {};
using EntityList = std::tuple<TestEntity>;
constexpr std::size_t NB_ENTITY_TYPES = std::tuple_size_v<EntityList>;
template <typename T>
concept Entity = ECS_HELPERS::is_type_in_tuple<EntityList, T>();
template <typename Tuple>
concept EntityTuple = ECS_HELPERS::is_superset<EntityList, Tuple>();

// The components list enabled for this entity
template <Entity E>
struct __EntityComponents;

// The input passed to create an entity
template <Entity E>
struct __EntityInputs;

// Specialize the structs above with every entitiy types

template <>
struct __EntityComponents<TestEntity> {
    using type = std::tuple<Position, Collision, Velocity, Groundable, PhysicsStats, CollisionDisplay, Camerable>;
};
template <>
struct __EntityInputs<TestEntity> {
    using type = Position;
};

// Convenience aliases so call sites stay clean:
// EntityComponents<TestEntity> instead of __EntityComponents<TestEntity>::type
template <Entity E>
using EntityComponents = typename __EntityComponents<E>::type;
template <Entity E>
using EntityInputs = typename __EntityInputs<E>::type;

template <Entity E>
constexpr ComponentSignature entity_signature = ECS_HELPERS::make_signature<ComponentList, EntityComponents<E>>();
template <ComponentTuple Tuple>
constexpr ComponentSignature tuple_signature = ECS_HELPERS::make_signature<ComponentList, Tuple>();

// -------------------------------------------------------------------------
// SYSTEMS
// -------------------------------------------------------------------------

template <ECS::Component... Cs>
class SystemBase;

class PhysicsSystem;
class WorldCollisionSystem;
class HitBoxDisplaySystem;
class CamerableSystem;
using SystemList = std::tuple<
    PhysicsSystem,
    WorldCollisionSystem,
    HitBoxDisplaySystem,
    CamerableSystem>;

using SystemId = std::uint8_t;
constexpr std::size_t NB_SYSTEMS = std::tuple_size_v<SystemList>;
template <typename T>
concept System = ECS_HELPERS::is_type_in_tuple<SystemList, T>();
template <typename Tuple>
concept SystemTuple = ECS_HELPERS::is_superset<SystemList, Tuple>();
template <System S>
constexpr SystemId system_id = static_cast<SystemId>(ECS_HELPERS::type_index<SystemList, S>());
template <typename F>
constexpr void for_each_systems(F&& f) {
    ECS_HELPERS::for_each_type_impl<SystemList, F>(
        std::forward<F>(f),
        std::make_index_sequence<NB_SYSTEMS>{});
}
}; // namespace ECS