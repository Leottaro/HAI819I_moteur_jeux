#pragma once

#include "Component.hpp"
#include "Entity.hpp"
#include "Systems.hpp"
#include "src/Camera.hpp"

// Outside the class — one specialisation per entity type
template <ECS::Entity E>
struct EntityFactory; // primary left undefined

template <>
struct EntityFactory<ECS::TestEntity> {
    static ECS::EntityId create(ComponentManager& cm, EntityManager& em, SystemManager& sm,
                                ECS::EntityInputs<ECS::TestEntity> inputs) {
        ECS::EntityId id = em.createEntity(ECS::entity_signature<ECS::TestEntity>);
        ECS::for_each_component_tuple<ECS::EntityComponents<ECS::TestEntity>>([&]<ECS::Component C>() {
            cm.addComponent<C>(id, C{});
        });

        // Entity-specific initialisation
        cm.getComponent<ECS::Position>(id) = inputs;
        cm.getComponent<ECS::Collision>(id).hitboxes = {AABB<float>(glm::vec3(-0.1f), glm::vec3(0.1f))};

        // Let this
        sm.onEntitySignatureChanged(cm, id, ECS::entity_signature<ECS::TestEntity>);
        return id;
    }
};

class ECSManager {
    ComponentManager cm;
    EntityManager em;
    SystemManager sm;

    // Internal helper: propagates the entity's new signature to the SystemManager
    // so systems update their entity lists accordingly.
    inline void onSignatureChanged(ECS::EntityId entity) {
        sm.onEntitySignatureChanged(cm, entity, em.entitySignature(entity));
    }

public:
    // -------------------------------------------------------------------------
    // Entity management
    // -------------------------------------------------------------------------

    // Single clean entry point — dispatches to the right EntityFactory specialisation
    template <ECS::Entity E>
    inline ECS::EntityId createEntity(ECS::EntityInputs<E> inputs) {
        return EntityFactory<E>::create(cm, em, sm, inputs);
    }

    inline bool hasEntity(ECS::EntityId entity) {
        return em.hasEntity(entity);
    }

    bool destroyEntity(ECS::EntityId entity) {
        if (em.destroyEntity(entity)) {
            cm.entityDestroyed(entity);
            sm.onEntitySignatureChanged(cm, entity, ECS::ComponentSignature{});
            return true;
        }
        return false;
    }

    // -------------------------------------------------------------------------
    // Component management
    // -------------------------------------------------------------------------

    // Attach a component to an entity. The component is value-initialised with
    template <ECS::Component C>
    void addComponent(ECS::EntityId entity, C component) {
        cm.addComponent<C>(entity, component);

        ECS::ComponentSignature& sig = em.entitySignature(entity);
        sig.set(ECS::component_id<C>);

        onSignatureChanged(entity);
    }

    // Detach a component from an entity and update bookkeeping.
    template <ECS::Component C>
    void removeComponent(ECS::EntityId entity) {
        cm.removeComponent<C>(entity);

        ECS::ComponentSignature& sig = em.entitySignature(entity);
        sig.reset(ECS::component_id<C>);

        onSignatureChanged(entity);
    }

    // Returns a reference to the component owned by the entity.
    // Behaviour is undefined if the entity does not own the component.
    template <ECS::Component C>
    inline C& getComponent(ECS::EntityId entity) {
        return cm.getComponent<C>(entity);
    }

    // -------------------------------------------------------------------------
    // System management
    // -------------------------------------------------------------------------

    // Returns a reference to the system of type S so callers can configure it
    // or invoke system-specific methods directly.
    template <ECS::System S>
    inline S& getSystem() {
        return sm.getSystem<S>();
    }

    // -------------------------------------------------------------------------
    // Update
    // -------------------------------------------------------------------------

    void update(float _deltaTime) {
        // PhysicsSystem: integrate forces and update velocities.
        sm.getSystem<ECS::PhysicsSystem>().update(cm, _deltaTime);

        // WorldCollisionSystem: sweep-and-slide.
        sm.getSystem<ECS::WorldCollisionSystem>().update(cm, _deltaTime);
    }

    void render(const Camera& _camera, ShaderProgram& _line_shader) {
        _line_shader.use();
        _line_shader.set("view", _camera.getViewMatrix());
        _line_shader.set("projection", _camera.getProjectionMatrix());

        // HitBoxDisplaySystem: render the hitobxes lines.
        sm.getSystem<ECS::HitBoxDisplaySystem>().render(cm, _line_shader);
    }
};