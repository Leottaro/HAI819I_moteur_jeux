#pragma once
#include "Data.hpp"

template <ECS::Component C>
class ECS::ComponentArray {
    std::bitset<ECS::MAX_ENTITIES> m_has_component{};
    std::array<C, ECS::MAX_ENTITIES> m_components{};
    size_t m_size{0};

public:
    ComponentArray() {}

    constexpr void insertData(ECS::EntityId entity, C component) {
        assert(!m_has_component[entity]);
        m_has_component[entity] = true;
        m_components[entity] = component;
        m_size++;
    }

    constexpr void removeData(ECS::EntityId entity) {
        assert(m_has_component[entity]);
        m_has_component[entity] = false;
        m_size--;
    }

    constexpr bool hasData(ECS::EntityId entity) { return m_has_component[entity]; }
    constexpr C& getData(ECS::EntityId entity) { return m_components[entity]; }
};

namespace COMPONENT_MANAGER_HELPERS {
// Helper to create the Component Arrays for every Component
template <std::size_t... I>
static constexpr auto make_component_arrays_impl(std::index_sequence<I...>) {
    return std::tuple<ECS::ComponentArray<std::tuple_element_t<I, ECS::ComponentList>>...>{};
}
using ComponentArrays = decltype(make_component_arrays_impl(std::make_index_sequence<ECS::NB_COMPONENTS>{}));
}; // namespace COMPONENT_MANAGER_HELPERS

class ECS::ComponentManager {
    COMPONENT_MANAGER_HELPERS::ComponentArrays m_component_arrays;

    // Convenience function to get the statically casted pointer to the ComponentArray of type T.
    template <ECS::Component C>
    constexpr ECS::ComponentArray<C>& getComponentArray() { return std::get<ECS::component_id<C>>(m_component_arrays); }

public:
    template <ECS::Component C>
    constexpr void addComponent(ECS::EntityId entity) {
        getComponentArray<C>().insertData(entity, C());
    }
    template <ECS::Component C>
    constexpr void addComponent(ECS::EntityId entity, C component) {
        getComponentArray<C>().insertData(entity, component);
    }

    template <ECS::Component C>
    constexpr void removeComponent(ECS::EntityId entity) {
        getComponentArray<C>().removeData(entity);
    }

    template <ECS::Component C>
    constexpr bool hasComponent(ECS::EntityId entity) {
        return getComponentArray<C>().hasData(entity);
    }

    template <ECS::Component C>
    constexpr C& getComponent(ECS::EntityId entity) {
        return getComponentArray<C>().getData(entity);
    }

    constexpr void entityDestroyed(ECS::EntityId entity) {
        ECS::for_each_components([&]<ECS::Component C>() {
            ComponentArray<C>& arr = getComponentArray<C>();
            if (arr.hasData(entity))
                arr.removeData(entity);
        });
    }
};
