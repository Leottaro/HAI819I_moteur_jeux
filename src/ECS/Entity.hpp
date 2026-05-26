#pragma once
#include "Data.hpp"

#include <functional>

class ECS::EntityManager {
    std::bitset<ECS::MAX_ENTITIES> m_entities{0};
    std::array<ECS::EntityId, ECS::MAX_ENTITIES> m_available_entities{};
    std::array<ECS::ComponentSignature, ECS::MAX_ENTITIES> m_signatures{};
    uint32_t m_nb_entities{0};

public:
    EntityManager() {
        for (ECS::EntityId e = 0; e < ECS::MAX_ENTITIES; e++)
            m_available_entities[e] = e;
    }

    inline ECS::EntityId createEntity(ECS::ComponentSignature signature) {
        ECS::EntityId id = m_available_entities[m_nb_entities++];
        m_entities[id] = true;
        m_signatures[id] = signature;
        return id;
    }

    inline bool hasEntity(ECS::EntityId entity) const {
        return m_entities[entity];
    }

    inline bool destroyEntity(ECS::EntityId entity) {
        if (!m_entities[entity])
            return false;
        m_signatures[entity].reset();
        m_available_entities[--m_nb_entities] = entity;
        m_entities[entity] = false;
        return true;
    }

    inline const ECS::ComponentSignature& entitySignature(ECS::EntityId entity) const {
        return m_signatures[entity];
    }

    inline void forEach(std::function<void(ECS::EntityId)> _f) const {
        ECS::EntityId entity = 0;
        size_t nb_entities = 0;
        while (nb_entities < m_nb_entities) {
            if (m_entities[entity]) {
                _f(entity);
                nb_entities++;
            }
            entity++;
        }
    }

    friend ECSManager;
};
