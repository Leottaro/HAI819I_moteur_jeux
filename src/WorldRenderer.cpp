#include "WorldRenderer.hpp"

// IMGUI
#include <imgui.h>

void WorldRenderer::setWorld(World* _world, const Camera& camera) {
    if (_world == nullptr)
        return;

    _world->m_added_chunks = std::list<Chunk*>();
    _world->m_removed_chunks = std::list<glm::ivec3>();

    // glm::vec3 cam_pos = _ecs_manager.getSystem<ECS::ControllableSystem>().getCamPos();
    m_render_chunks.clear();
    _world->m_chunks.forEach([&](Chunk& chunk) {
        m_render_chunks.emplace(chunk.getPos(), &chunk, camera.getCamPos());
    });
}

void WorldRenderer::updateWorld(World* _world) {
    m_removed_chunks.insert(m_removed_chunks.end(), _world->m_removed_chunks.begin(), _world->m_removed_chunks.end());
    m_added_chunks.insert(m_added_chunks.end(), _world->m_added_chunks.begin(), _world->m_added_chunks.end());
    _world->m_removed_chunks.clear();
    _world->m_added_chunks.clear();

    float angle = (_world->m_world_time / DAY_LENGTH) * 2.f * M_PIf;
    float t = (sin(angle) + 1.0f) * 0.5f;
    m_sky_color = glm::mix(SKY_NIGHT, SKY_DAY, t);
    m_sun_color = glm::mix(SUN_NOON, SUN_DUSK, t);
    m_last_time = _world->m_world_time;
}

void WorldRenderer::renderChunks(ShaderProgram& _chunk_shader, const Camera& camera) {
    glm::vec2 sun_angle(m_sun_season, m_last_time * TIME_ANGLE_FACTOR); // angle du soleil (nord<->sud, est<->ouest)
    glm::vec3 sun_direction(sin(sun_angle.x), cos(sun_angle.x) * sin(sun_angle.y), cos(sun_angle.x) * cos(sun_angle.y));

    const Camera::Frustum& frustum = camera.getFrustum();
    const glm::vec3& cam_pos = camera.getCamPos();

    for (const glm::ivec3& chunk_pos : m_removed_chunks)
        m_render_chunks.remove(chunk_pos);
    for (Chunk* chunk : m_added_chunks)
        m_render_chunks.emplace(chunk->getPos(), chunk, cam_pos);
    m_removed_chunks.clear();
    m_added_chunks.clear();

    glm::ivec3 cam_chunk = Chunk::posToChunkPos(cam_pos);
    bool rebuild_all = cam_chunk != m_last_cam_chunk;

    m_render_chunks.forEach([rebuild_all, cam_pos, cam_chunk](ChunkRenderer& chunk_renderer) {
        if (rebuild_all)
            chunk_renderer.shouldRebuildMesh() = true;

        if (rebuild_all ||
            chunk_renderer.shouldRebuildMesh() ||
            cam_chunk.x == chunk_renderer.getPos().x ||
            cam_chunk.y == chunk_renderer.getPos().y ||
            cam_chunk.z == chunk_renderer.getPos().z) {
            chunk_renderer.updateShaderData(cam_pos);
        }
    });
    m_last_cam_chunk = cam_chunk;

    std::vector<std::pair<float, const ChunkRenderer*>> draw_list;
    draw_list.reserve(m_render_chunks.size());

    m_render_chunks.forEach([&draw_list, frustum, cam_pos](const ChunkRenderer& chunk_renderer) {
        if ((chunk_renderer.getOpaqueTriangles() > 0 || chunk_renderer.getTranslucentTriangles() > 0) // on skip les chunks sans triangles
            && frustum.isVisible(chunk_renderer.getAABB())                                            // On skip les chunk hors du frustum
        ) {
            float dist = glm::distance(cam_pos, glm::vec3(chunk_renderer.getPos()) + glm::vec3(Chunk::CHUNK_SIZE * 0.5f));
            draw_list.push_back({dist, &chunk_renderer});
        }
    });
    std::sort(draw_list.begin(), draw_list.end(),
              [](auto& a, auto& b) { return a.first > b.first; });

    // --- PASS 3: draw (sequential pointer chasing, unavoidable) ---
    _chunk_shader.use();
    _chunk_shader.set("view", camera.getView());
    _chunk_shader.set("projection", camera.getProjection());
    _chunk_shader.set("camera_pos", cam_pos);
    _chunk_shader.set("sun_direction", sun_direction);
    _chunk_shader.set("sun_color", m_sun_color);
    _chunk_shader.set("albedo_atlas", 0);
    _chunk_shader.set("normal_atlas", 1);
    _chunk_shader.set("specular_atlas", 2);

    for (auto [_, chunk_renderer] : draw_list) {
        if (chunk_renderer->getOpaqueTriangles() > 0) {
            _chunk_shader.set("chunk_pos", chunk_renderer->getPos());
            chunk_renderer->renderOpaque();
        }
    }
    glDepthMask(GL_FALSE);
    glEnable(GL_BLEND);
    for (auto& [_, chunk_renderer] : draw_list) {
        if (chunk_renderer->getTranslucentTriangles() > 0) {
            _chunk_shader.set("chunk_pos", chunk_renderer->getPos());
            chunk_renderer->renderTranslucent();
        }
    }
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}
// void WorldRenderer::renderDebugBoxes(ShaderProgram& _line_shader, ECSManager* _ecs_manager) const {
//     const ECS::ControllableSystem& controllable_system = _ecs_manager.getSystem<ECS::ControllableSystem>();
//     _line_shader.use();
//     _line_shader.set("view", controllable_system.getView());
//     _line_shader.set("projection", controllable_system.getProjection());
//     _line_shader.set("color", glm::vec3(1.f));

//     AABB<float> chunk_hitbox(glm::vec3(0), glm::vec3(Chunk::CHUNK_SIZE));
//     AABBRenderer box(chunk_hitbox);
//     m_render_chunks.forEach([&_line_shader, &box](const ChunkRenderer& chunk_renderer) {
//         _line_shader.set("position", glm::vec3(chunk_renderer.getPos()));
//         box.render();
//     });
//     box.clearShaderData();

//     chunk_hitbox.min = glm::vec3(Chunk::CHUNK_SIZE / 4);
//     chunk_hitbox.max = glm::vec3(3 * Chunk::CHUNK_SIZE / 4);
//     box.initShaderData(chunk_hitbox);
//     for (const glm::ivec3& chunk_pos : _world->m_chunks_frontier) {
//         _line_shader.set("position", glm::vec3(chunk_pos));
//         box.render();
//     }
//     box.clearShaderData();
// }

// void WorldRenderer::updateInterface(Window& _window, World* _world) {
//     if (!ImGui::Begin("World Info")) {
//         ImGui::End();
//         return;
//     }

//     _ecs_manager.updateInterfaces(_window);

//     int current_type = static_cast<int>(_world->m_gentype);
//     if (ImGui::Combo("Generation Type", &current_type, GenTypeNames, IM_ARRAYSIZE(GenTypeNames))) {
//         _world->m_gentype = static_cast<GenType>(current_type);
//         m_render_chunks.clear();
//         _world->clear();
//     }

//     if (ImGui::InputInt("Render distance", &_world->m_render_distance, 1, 2)) {
//         _world->m_render_distance = std::max(_world->m_render_distance, 1);
//     }

//     ImGui::Spacing();
//     ImGui::Separator();
//     ImGui::Spacing();

//     ImGui::DragScalar("World Time", ImGuiDataType_U64, &_world->m_world_time, 10.0f, 0, &DAY_LENGTH);
//     if (ImGui::DragFloat("World Season", &m_sun_season, 0.01f, -M_2_PIf, M_2_PIf)) {
//         m_sun_season = std::clamp(m_sun_season, -M_2_PIf, M_2_PIf);
//     }
//     if (ImGui::Button(_world->m_play ? "Pause" : "Play")) {
//         _world->m_play = !_world->m_play;
//     }
//     ImGui::End();
// }
