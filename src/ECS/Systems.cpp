#include "Data.hpp"
#include "Component.hpp"
#include "Entity.hpp"
#include <unordered_set>

void bounce(float _static_friction, float _friction, float _restitution, const glm::vec3& _normal, glm::vec3& _vel) {
    float v_dot_n = glm::dot(_vel, _normal);
    glm::vec3 v_normal = v_dot_n * _normal;
    glm::vec3 v_tangent = _vel - v_normal;
    v_tangent *= 1.f - _friction;
    if (glm::length(v_tangent) < _static_friction * glm::length(v_normal)) {
        v_tangent *= 0.f;
    }
    v_normal *= -_restitution;
    if (glm::length(v_normal) < 0.05f) {
        v_normal = glm::vec3(0.f);
    }
    _vel = v_normal + v_tangent;
}
struct CollisionsInfos {
    float t;
    glm::vec3 normal;
    glm::vec3 pos;
    Block* block;
};
// Return true if it detected a collision
bool detectCollision(float _deltaTime, CollisionsInfos& res, ECS::Position& _position, ECS::BoundingBox& _bounding_box, ECS::Velocity& _velocity) {
    res.t = std::numeric_limits<float>::max();

    std::vector<Block*> to_explore;
    for (const AABB<float>& hitbox : _bounding_box.hitboxes) {
        glm::ivec3 min_block = Block::posToBlockPos(_position.pos + hitbox.min);
        glm::ivec3 max_block = Block::posToBlockPos(_position.pos + hitbox.max);
        for (uint y : {min_block.y - 1, min_block.y, max_block.y, max_block.y + 1})
            for (uint z : {min_block.z - 1, min_block.z, max_block.z, max_block.z + 1})
                for (uint x : {min_block.x - 1, min_block.x, max_block.x, max_block.x + 1})
                    to_explore.push_back(_position.current_chunk->findBlock(glm::ivec3(x, y, z)));
    }

    std::set<Block*> explored;
    while (!to_explore.empty()) {
        Block* block = to_explore.back();
        to_explore.pop_back();
        if (!explored.insert(block).second)
            continue;

        glm::ivec3 block_pos = block->getPos();
        AABB<float> block_aabb(block_pos, block_pos + glm::ivec3(1));
        // std::cout << "\t\t" << size_t(block->getType()) << " at " << glm::to_string(block_pos) << std::endl;

        bool clip = false;
        for (const AABB<float>& hitbox : _bounding_box.hitboxes) {
            glm::vec3 dist;
            if (block_aabb.intersectAABB(_position.pos + hitbox, dist)) {
                clip = true;
                continue;
            }

            float t;
            glm::vec3 normal(0.);
            if (block->hasHitbox() && block_aabb.intersectAABB(_position.pos + hitbox, _deltaTime * _velocity.vel, t, normal)) {
                if (t < res.t) {
                    res.t = t;
                    res.normal = normal;
                    res.block = block;
                    res.pos = _position.pos + t * _deltaTime * _velocity.vel;
                    while (block_aabb.intersectAABB(res.pos + hitbox, dist)) {
                        res.pos += dist;
                    }
                }
                // std::cout << "\t\t\tintersection: " << "t=" << res.t << "\tnormal=" << glm::to_string(res.normal) << "\tpos=" << glm::to_string(res.pos) << std::endl;
            }
        }

        if (clip) {
            // std::cout << "\t\tclip" << std::endl;
            for (int face_i = 0; face_i < 6; face_i++)
                if (block->m_neighbours[face_i] != nullptr && explored.find(block->m_neighbours[face_i]) == explored.end())
                    to_explore.push_back(block->m_neighbours[face_i]);
        }
    }

    return res.t <= 1.f;
}

template <ECS::Component... Cs>
class ECS::SystemBase {
public:
    // Built entirely at compile time from the component pack
    static constexpr ECS::ComponentSignature signature = []() {
        ECS::ComponentSignature sig{};
        (sig.set(ECS::component_id<Cs>), ...);
        return sig;
    }();

    std::unordered_set<ECS::EntityId> m_entities{};
};

class ECS::PhysicsSystem : public ECS::SystemBase<ECS::Position, ECS::Velocity, ECS::Groundable, ECS::PhysicsStats> {
public:
    void update(ComponentManager& cm, float _deltaTime) {
        for (ECS::EntityId entity : m_entities) {
            ECS::Position& position = cm.getComponent<ECS::Position>(entity);
            ECS::Velocity& velocity = cm.getComponent<ECS::Velocity>(entity);
            ECS::Groundable& groundable = cm.getComponent<ECS::Groundable>(entity);
            ECS::PhysicsStats& stats = cm.getComponent<ECS::PhysicsStats>(entity);

            std::vector<glm::vec3> forces;
            forces.reserve(3);
            if (groundable.on_ground) {
                Block* in_block = position.current_chunk->getBlock(position.pos);
                Block* under_block = in_block->m_neighbours[2]; // 2 -> -y
                if (under_block == nullptr || !under_block->hasHitbox()) {
                    groundable.on_ground = false;
                } else {
                    float ground_friction = under_block->getCollisionStats()[0];
                    velocity.vel *= ground_friction - 1.f;
                }
            } else {
                forces.push_back(glm::vec3(0.f, -9.81f, 0.f) * stats.weight); // g
                Block* block = position.current_chunk->getBlock(position.pos);
                float densite_fluide = block->getDensity();
                if (densite_fluide > 0.f) {
                    forces.push_back(densite_fluide * -forces[0] * stats.volume / (stats.weight / stats.volume)); // flottaison
                    forces.push_back(densite_fluide * velocity.vel * -stats.drag);                                // drag
                }
            }

            // Add forces
            glm::vec3 m_accel(0.f, 0.f, 0.f);
            for (const glm::vec3& force : forces) {
                m_accel += force;
            }
            m_accel /= stats.weight;
            velocity.vel += _deltaTime * m_accel;
        }
    }
};

class ECS::WorldCollisionSystem : public ECS::SystemBase<ECS::Position, ECS::BoundingBox, ECS::Velocity, ECS::Groundable> {
    // return if the
    bool updateEntity(ComponentManager& cm, float _deltaTime, ECS::EntityId _entity) {
        ECS::Position& position = cm.getComponent<ECS::Position>(_entity);
        ECS::BoundingBox& bounding_box = cm.getComponent<ECS::BoundingBox>(_entity);
        ECS::Velocity& velocity = cm.getComponent<ECS::Velocity>(_entity);
        ECS::Groundable& groundable = cm.getComponent<ECS::Groundable>(_entity);

        // Chunk collision detection
        CollisionsInfos collision;
        // std::cout << std::endl
        //           << std::endl
        //           << std::endl
        //           << "Starting detection: dt=" << _deltaTime << std::endl;
        while (detectCollision(_deltaTime, collision, position, bounding_box, velocity)) {
            // std::cout << "\tCOLLISION: " << std::endl
            //           << "\t\t" << size_t(collision.block->getType()) << " at " << glm::to_string(collision.block->getPos()) << std::endl
            //           << "\t\tt: " << collision.t << std::endl
            //           << "\t\tnormal: " << glm::to_string(collision.normal) << std::endl
            //           << "\t\told pos: " << glm::to_string(position.pos) << std::endl
            //           << "\t\told vel: " << glm::to_string(velocity.vel) << std::endl;

            float friction = collision.block->getCollisionStats()[0];
            float restitution = collision.block->getCollisionStats()[1];
            bounce(0.f, friction, restitution, collision.normal, velocity.vel);
            if (collision.normal == glm::vec3(0.f, 1.f, 0.f) && velocity.vel.y == 0.f) {
                groundable.on_ground = true;
            }
            position.pos = collision.pos;
            position.current_chunk = position.current_chunk->getChunk(position.pos);
            if (position.current_chunk == nullptr) {
                return false;
            }
            _deltaTime *= (1.f - collision.t);
            // std::cout
            //     << "\t\tpos: " << glm::to_string(position.pos) << std::endl
            //     << "\t\tvel: " << glm::to_string(velocity.vel) << std::endl
            //     << "dt=" << _deltaTime << std::endl;
        }
        position.pos += _deltaTime * velocity.vel;
        // std::cout << "\tAPPLIED dt=" << _deltaTime << " velocity" << std::endl
        //           << "\t\tnew pos: " << glm::to_string(position.pos) << std::endl
        //           << "\t\tnew vel: " << glm::to_string(velocity.vel) << std::endl;

        position.current_chunk = position.current_chunk->getChunk(position.pos);
        if (position.current_chunk == nullptr) {
            return false;
        }

        return true;
    }

public:
    void update(ComponentManager& cm, EntityManager& em, float _deltaTime) {
        for (ECS::EntityId entity : m_entities)
            if (!updateEntity(cm, _deltaTime, entity))
                em.DestroyEntity(entity);
    }
};

class SystemManager {
    ECS::SystemList m_systems;

public:
    // Convenience function to get the statically casted pointer to the System of type T.
    template <ECS::System S>
    constexpr S& getSystem() { return std::get<ECS::system_id<S>>(m_systems); }

    // Called whenever an entity's signature changes (add/remove component, destroy)
    void onEntitySignatureChanged(ECS::EntityId _entity, ECS::ComponentSignature _entity_sig) {
        ECS::for_each_system([&]<ECS::System S>() {
            S& system = getSystem<S>();
            if ((_entity_sig & system.signature) == system.signature) {
                system.m_entities.insert(_entity);
            } else {
                system.m_entities.erase(_entity);
            }
        });
    }
};