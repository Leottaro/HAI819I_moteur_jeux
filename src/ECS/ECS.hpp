#pragma once
#include "Data.hpp"
#include "Component.hpp"
#include "Entity.hpp"
#include "Systems.hpp"

// Outside the class — one specialisation per entity type
template <ECS::EntityType E>
struct EntityFactory; // primary left undefined

template <>
struct EntityFactory<ECS::TestEntity> {
    static constexpr ECS::EntityId create(ECS::ComponentManager& cm, ECS::EntityManager& em, ECS::SystemManager& sm,
                                          ECS::EntityTypeInputs<ECS::TestEntity> inputs) {
        ECS::EntityId id = em.createEntity(ECS::entity_signature<ECS::TestEntity>);
        ECS::for_each_component_tuple<ECS::EntityTypeComponents<ECS::TestEntity>>([&]<ECS::Component C>() {
            cm.addComponent<C>(id, C{});
        });

        // Entity-specific initialisation
        cm.getComponent<ECS::Positionnable>(id) = inputs;
        cm.getComponent<ECS::Collisionnable>(id).hitboxes = {AABB<float>(glm::vec3(-1.f / 3.f, 0.f, -1.f / 3.f), glm::vec3(1.f / 3.f, 1.74f, 1.f / 3.f))};
        cm.getComponent<ECS::Camerable>(id).eye_pos = glm::vec3(0.f, 1.5f, 0.f);

        // Let this
        sm.onEntitySignatureChanged(cm, id, ECS::entity_signature<ECS::TestEntity>);
        return id;
    }
};

class ECSManager {
    ECS::ComponentManager cm;
    ECS::EntityManager em;
    ECS::SystemManager sm;

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
    template <ECS::EntityType E>
    inline ECS::EntityId createEntity(ECS::EntityTypeInputs<E> inputs) {
        return EntityFactory<E>::create(cm, em, sm, inputs);
    }

    inline bool hasEntity(ECS::EntityId entity) {
        return em.hasEntity(entity);
    }

    inline bool destroyEntity(ECS::EntityId entity) {
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
    inline void addComponent(ECS::EntityId entity, C component) {
        cm.addComponent<C>(entity, component);

        ECS::ComponentSignature& sig = em.entitySignature(entity);
        sig.set(ECS::component_id<C>);

        onSignatureChanged(entity);
    }

    // Detach a component from an entity and update bookkeeping.
    template <ECS::Component C>
    inline void removeComponent(ECS::EntityId entity) {
        cm.removeComponent<C>(entity);

        ECS::ComponentSignature& sig = em.entitySignature(entity);
        sig.reset(ECS::component_id<C>);

        onSignatureChanged(entity);
    }

    template <ECS::Component C>
    inline bool hasComponent(ECS::EntityId entity) {
        return cm.hasComponent<C>(entity);
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

    inline void startControl(Window& _window, ECS::EntityId entity) {
        getSystem<ECS::CamerableSystem>().startControl(cm, entity, _window);
        _window.keyboard.bind(GLFW_KEY_C, [&]() { getSystem<ECS::CamerableSystem>().toggleControlType(cm); }, nullptr);
    }
    inline void stopControl(Window& _window) {
        getSystem<ECS::CamerableSystem>().stopControl();
    }

    void update(Window& window, float _dt) {
        // PositionSystem: delete every out of world entities
        std::vector<ECS::EntityId> entities_to_destroy = getSystem<ECS::PositionSystem>().getOutOfBoundEntities(cm);
        for (ECS::EntityId entity : entities_to_destroy)
            destroyEntity(entity);

        // ControllingSystem: control entities
        if (getSystem<ECS::CamerableSystem>().getControlType() != ECS::ControlType::FreeCam) {
            getSystem<ECS::ControllingSystem>().update(cm, window, _dt);
        }

        // PhysicsSystem: integrate forces and update velocities.
        getSystem<ECS::PhysicsSystem>().update(cm, _dt);

        // WorldCollisionSystem: sweep-and-slide.
        getSystem<ECS::WorldCollisionSystem>().update(cm, _dt);

        // CamerableSystem: update camera
        getSystem<ECS::CamerableSystem>().update(cm, window, _dt);
    }

    void render(ShaderProgram& _line_shader) {
        _line_shader.use();
        _line_shader.set("view", getSystem<ECS::CamerableSystem>().getView());
        _line_shader.set("projection", getSystem<ECS::CamerableSystem>().getProjection());

        // HitBoxDisplaySystem: render the hitobxes lines.
        getSystem<ECS::HitBoxDisplaySystem>().render(cm, _line_shader);
    }
};