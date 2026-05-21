#pragma once

// USUAL INCLUDES
class World;
#include "src/Window.hpp"
#include "src/Frustum.hpp"
#include "src/Transformation.hpp"
#include "src/AABB.hpp"
#include "src/ShaderProgram.hpp"
#include "src/Helpers.hpp"

#include <bitset>
#include <vector>
#include <unordered_set>

// Inspired by https://austinmorlan.com/posts/entity_component_system/

namespace ECS {
using ComponentId = std::uint8_t;
using EntityId = std::uint16_t;
constexpr EntityId MAX_ENTITIES = 65535;
constexpr glm::vec3 G{0.f, -100.f / 3.f, 0.f};

enum class ControlType {
    FirstPerson = 0,
    ThirdPerson,
    FreeCam,
    __COUNT
};
constexpr size_t NB_CONTROL_TYPES = static_cast<size_t>(ControlType::__COUNT);

// -------------------------------------------------------------------------
// COMPONENTS
// -------------------------------------------------------------------------
struct Positionnable {
    World* current_world{nullptr};
    glm::vec3 pos{0.f, 0.f, 0.f};
};
struct Collisionnable {
    std::vector<AABB<float>> hitboxes{};
};
struct Movable {
    glm::vec3 vel{0.f, 0.f, 0.f};
};
struct Groundable {
    bool on_ground{false};
    float air_control_speed{0.1f};
    float walk_speed{3.0f};
    float jump_force{10.0f};
};
struct PhysicsStats {
    float weight{1500.f};
    float volume{1.f};
    float drag{1.05f};
};
struct CollisionDisplay {
    std::vector<AABBRenderer> boxes{};
};
struct Orientable {
    glm::vec2 orientation{0.f, 0.f};
};
struct Camerable {
    glm::vec3 eye_pos{0.f};        // Only in first and third person
    float distance_to_center{5.f}; // Only in third person
};
struct Controllable {
    ControlType type;
};

// L'ensemble des composants, tout le reste est dérivé automatiquement
using ComponentList = std::tuple<
    Positionnable,
    Collisionnable,
    Movable,
    Groundable,
    PhysicsStats,
    CollisionDisplay,
    Orientable,
    Camerable,
    Controllable>;

// Le nombre total de composants
constexpr std::size_t NB_COMPONENTS = std::tuple_size_v<ComponentList>;

// Un enseble de bits: le i-ème bit décrit si l'entité "possède" le composant i
// Permet de savoir quels composants a une entitée
using ComponentSignature = std::bitset<NB_COMPONENTS>;

// Permet de templater un Component en vérifiant si il est dans ComponentList
// S'uttilise dans un template a la place d'un typename
template <typename T>
concept Component = ECS_HELPERS::is_type_in_tuple<ComponentList, T>();

// Permet de templater un Tuple de component en vérifiant si ComponentList est son superset
// S'uttilise dans un template a la place d'un typename
template <typename Tuple>
concept ComponentTuple = ECS_HELPERS::is_superset<ComponentList, Tuple>();

// Permet de récuper l'indice d'un composant dans ComponentList
// S'uttilise comme ça: ECS::component_id<T>
template <Component C>
constexpr ComponentId component_id = static_cast<ComponentId>(ECS_HELPERS::type_index<ComponentList, C>());

// Permet d'éxécuter une fonction pour tout types de composants
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

template <ECS::Component C>
class ComponentArray;
class ComponentManager;

// -------------------------------------------------------------------------
// ENTITIES
// -------------------------------------------------------------------------

struct TestEntity {};

using EntityTypeList = std::tuple<TestEntity>;
constexpr std::size_t NB_ENTITY_TYPES = std::tuple_size_v<EntityTypeList>;
template <typename T>
concept EntityType = ECS_HELPERS::is_type_in_tuple<EntityTypeList, T>();
template <typename Tuple>
concept EntityTypeTuple = ECS_HELPERS::is_superset<EntityTypeList, Tuple>();

// The components list enabled for this entity
template <EntityType E>
struct __EntityTypeComponents;

// The input passed to create an entity
template <EntityType E>
struct __EntityTypeInputs;

// Specialize the structs above with every entitiy types

template <>
struct __EntityTypeComponents<TestEntity> {
    using type = std::tuple<Positionnable, Collisionnable, Movable, Groundable, PhysicsStats, CollisionDisplay, Orientable, Camerable, Controllable>;
};
template <>
struct __EntityTypeInputs<TestEntity> {
    using type = struct Input : Positionnable {};
};

// Convenience aliases so call sites stay clean:
// EntityTypeComponents<TestEntity> instead of __EntityTypeComponents<TestEntity>::type
template <EntityType E>
using EntityTypeComponents = typename __EntityTypeComponents<E>::type;
template <EntityType E>
using EntityTypeInputs = typename __EntityTypeInputs<E>::type;

template <EntityType E>
constexpr ComponentSignature entity_signature = ECS_HELPERS::make_signature<ComponentList, EntityTypeComponents<E>>();
template <ComponentTuple Tuple>
constexpr ComponentSignature tuple_signature = ECS_HELPERS::make_signature<ComponentList, Tuple>();

class EntityManager;

// -------------------------------------------------------------------------
// SYSTEMS
// -------------------------------------------------------------------------

class PositionSystem;
class PhysicsSystem;
class WorldCollisionSystem;
class HitBoxDisplaySystem;
class CamerableSystem;
class ControllingSystem;
using SystemList = std::tuple<
    PositionSystem,
    PhysicsSystem,
    WorldCollisionSystem,
    HitBoxDisplaySystem,
    CamerableSystem,
    ControllingSystem>;

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

class SystemManager;

}; // namespace ECS

class ECSManager;