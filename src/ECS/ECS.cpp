#include "ECS.hpp"
#include "src/World.hpp"
#include "src/Frustum.hpp"
#include "src/Transformation.hpp"
#include "src/Window.hpp"
#include "src/ShaderProgram.hpp"

bool WORLD_API::isChunkLoaded(ECS::Positionnable& positionnable) {
    return positionnable.current_world->isChunkLoaded(Chunk::posToChunkPos(positionnable.pos));
}
Block* WORLD_API::findBlock(ECS::Positionnable& positionnable, const glm::ivec3& _block_pos) {
    return positionnable.current_world->findBlock(_block_pos);
}
