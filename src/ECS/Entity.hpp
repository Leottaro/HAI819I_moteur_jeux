#pragma once

#include "Data.hpp"

class EntityManager {
    std::bitset<ECS::MAX_ENTITIES> m_entities{0};
    std::queue<ECS::EntityId> m_available_entities{};
    std::array<ECS::ComponentSignature, ECS::MAX_ENTITIES> m_signatures{};
    uint32_t m_nb_entities{};

public:
    EntityManager() {
        for (ECS::EntityId entity = 0; entity < ECS::MAX_ENTITIES; ++entity)
            m_available_entities.push(entity);
    }

    inline ECS::EntityId CreateEntity() {
        ECS::EntityId id = m_available_entities.front();
        m_available_entities.pop();
        m_entities[id] = true;
        m_nb_entities++;
        return id;
    }
    inline ECS::EntityId CreateEntity(ECS::ComponentSignature signature) {
        ECS::EntityId id = CreateEntity();
        m_signatures[id] = signature;
        return id;
    }
    inline ECS::EntityId CreateEntity(ECS::EntityType type) {
        return CreateEntity(ECS::GET_SIGNATURE(type));
    }

    inline void DestroyEntity(ECS::EntityId entity) {
        m_signatures[entity].reset();
        m_available_entities.push(entity);
        m_entities[entity] = false;
        m_nb_entities--;
    }

    inline void SetEntitySignature(ECS::EntityId entity, ECS::ComponentSignature signature) {
        m_signatures[entity] = signature;
    }

    inline ECS::ComponentSignature GetEntitySignature(ECS::EntityId entity) const {
        return m_signatures[entity];
    }
};
