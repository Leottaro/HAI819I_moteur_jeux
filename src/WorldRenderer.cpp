#include "WorldRenderer.hpp"

// IMGUI
#include <imgui.h>

void WorldRenderer::clear() {
    m_added_chunks.clear();
    m_removed_chunks.clear();
    m_last_frontier.clear();
    m_render_chunks.clear();
}

void WorldRenderer::setWorld(World* _world, const Camera& camera) {
    if (_world == nullptr)
        return;

    _world->m_added_chunks = std::list<Chunk*>();
    _world->m_removed_chunks = std::list<glm::ivec3>();
    m_last_frontier = _world->m_chunks_frontier;

    // glm::vec3 cam_pos = _ecs_manager.getSystem<ECS::ControllableSystem>().getCamPos();
    m_render_chunks.clear();
    _world->m_chunks.forEach([&](Chunk& chunk) {
        m_render_chunks.emplace(chunk.getPos(), &chunk, camera.getCamPos());
    });
}

void WorldRenderer::updateWorld(World* _world) {
    m_removed_chunks.insert(m_removed_chunks.end(), _world->m_removed_chunks.begin(), _world->m_removed_chunks.end());
    m_added_chunks.insert(m_added_chunks.end(), _world->m_added_chunks.begin(), _world->m_added_chunks.end());
    m_last_frontier = _world->m_chunks_frontier;

    _world->m_removed_chunks.clear();
    _world->m_added_chunks.clear();
    m_last_time = _world->m_world_time;

    float angle = (static_cast<float>(_world->m_world_time % DAY_LENGTH) / DAY_LENGTH) * 2.f * M_PIf;
    float t = (sin(angle) + 1.f) * 0.5f;
    float tbis = (sin(angle * 2.f + M_PI_4f) + 1.f) * 0.5f;
    m_sky_color = glm::mix(SKY_NIGHT, SKY_DAY, t);
    m_sun_color = glm::mix(SUN_NOON, SUN_DUSK, tbis);

    glm::vec2 sun_angle(m_sun_season, m_last_time * TIME_ANGLE_FACTOR);
    // https://en.wikipedia.org/wiki/N-vector
    m_sun_direction = glm::vec3(sin(sun_angle.x), cos(sun_angle.x) * sin(sun_angle.y), cos(sun_angle.x) * cos(sun_angle.y));
}

void WorldRenderer::renderChunks(ShaderProgram& _chunk_shader, const Camera::Frustum& _frustum, const glm::vec3& _cam_pos) {
    for (const glm::ivec3& chunk_pos : m_removed_chunks)
        m_render_chunks.remove(chunk_pos);
    for (Chunk* chunk : m_added_chunks)
        m_render_chunks.emplace(chunk->getPos(), chunk, _cam_pos);
    m_removed_chunks.clear();
    m_added_chunks.clear();

    glm::ivec3 cam_chunk = Chunk::posToChunkPos(_cam_pos);

    m_render_chunks.forEach([_cam_pos, cam_chunk](ChunkRenderer& chunk_renderer) {
        if (chunk_renderer.shouldRebuildMesh() ||
            (chunk_renderer.getTranslucentTriangles() > 1 && (cam_chunk.x == chunk_renderer.getPos().x ||
                                                              cam_chunk.y == chunk_renderer.getPos().y ||
                                                              cam_chunk.z == chunk_renderer.getPos().z))) {
            chunk_renderer.updateShaderData(_cam_pos);
        }
    });
    m_last_cam_chunk = cam_chunk;

    std::vector<const ChunkRenderer*> draw_list;
    std::vector<float> draw_list_distances2;
    draw_list.reserve(m_render_chunks.size());
    draw_list_distances2.reserve(m_render_chunks.size());
    m_render_chunks.forEach([&draw_list, &draw_list_distances2, _frustum, _cam_pos](const ChunkRenderer& chunk_renderer) {
        if ((chunk_renderer.getOpaqueTriangles() > 0 || chunk_renderer.getTranslucentTriangles() > 0) // on skip les chunks sans triangles
            && _frustum.isVisible(chunk_renderer.getAABB())                                           // On skip les chunk hors du frustum
        ) {
            float dist_squared = glm::distance2(_cam_pos, glm::vec3(chunk_renderer.getPos()) + glm::vec3(Chunk::CHUNK_SIZE * 0.5f));
            auto dist_it = std::lower_bound(draw_list_distances2.begin(), draw_list_distances2.end(), dist_squared, std::greater<float>());
            const size_t i = static_cast<size_t>(std::distance(draw_list_distances2.begin(), dist_it));

            draw_list_distances2.insert(dist_it, dist_squared);
            draw_list.insert(draw_list.begin() + i, &chunk_renderer);
        }
    });

    // --- PASS 3: draw (sequential pointer chasing, unavoidable) ---
    for (const ChunkRenderer* chunk_renderer : draw_list) {
        if (chunk_renderer->getOpaqueTriangles() > 0) {
            _chunk_shader.set("chunk_pos", chunk_renderer->getPos());
            chunk_renderer->renderOpaque();
        }
    }
    glDepthMask(GL_FALSE);
    glEnable(GL_BLEND);
    for (const ChunkRenderer* chunk_renderer : draw_list) {
        if (chunk_renderer->getTranslucentTriangles() > 0) {
            _chunk_shader.set("chunk_pos", chunk_renderer->getPos());
            chunk_renderer->renderTranslucent();
        }
    }
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}
void WorldRenderer::renderDebugBoxes(ShaderProgram& _line_shader) const {
    AABB<float> chunk_hitbox(glm::vec3(0), glm::vec3(Chunk::CHUNK_SIZE));
    AABBRenderer box(chunk_hitbox);
    m_render_chunks.forEach([&_line_shader, &box](const ChunkRenderer& chunk_renderer) {
        _line_shader.set("position", glm::vec3(chunk_renderer.getPos()));
        box.render();
    });

    chunk_hitbox.min = glm::vec3(Chunk::CHUNK_SIZE / 4);
    chunk_hitbox.max = glm::vec3(3 * Chunk::CHUNK_SIZE / 4);
    box.upateAABB(chunk_hitbox);
    for (const glm::ivec3& chunk_pos : m_last_frontier) {
        _line_shader.set("position", glm::vec3(chunk_pos));
        box.render();
    }
}

bool WorldRenderer::updateInterface(World* _world, const std::vector<glm::vec3>& world_gen_pos) {
    bool cleared = false;
    if (!ImGui::Begin("World Info")) {
        ImGui::End();
        return cleared;
    }

    int current_type = static_cast<int>(_world->m_gentype);
    if (ImGui::Combo("Generation Type", &current_type, GenTypeNames, IM_ARRAYSIZE(GenTypeNames))) {
        _world->m_gentype = static_cast<GenType>(current_type);
        clear();
        _world->clear();
        cleared = true;
    }

    uint render_distance = _world->m_render_distance;
    if (ImGui::InputScalar("Render distance", ImGuiDataType_U32, &render_distance)) {
        _world->m_render_distance = std::max(render_distance, 2U);
    }

    if (ImGui::DragFloat("Shadowmap radius", &m_shadowmap_radius, 0.5f, 1.f)) {
        m_shadowmap_radius = std::max(m_shadowmap_radius, 1.f);
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::DragScalar("World Time", ImGuiDataType_U64, &_world->m_world_time, 10.0f, 0, &DAY_LENGTH);
    if (ImGui::DragFloat("World Season", &m_sun_season, 0.01f, -M_PI_2f, M_PI_2f)) {
        m_sun_season = std::clamp(m_sun_season, -M_PI_2f, M_PI_2f);
    }
    if (ImGui::Button(_world->m_play ? "Pause" : "Play")) {
        _world->m_play = !_world->m_play;
    }

    ImGui::Text("Seed actuelle : %d", _world->getWorldSeed());

    static char new_seed[64];
    ImGui::InputText("New Seed", new_seed, sizeof(new_seed));
    ImGui::SameLine();
    if (ImGui::Button("Regen")) {
        int new_seed_val;
        std::string_view new_seed_str(new_seed);
        std::cout << "current string : " << new_seed << "\n";

        if (new_seed_str.length() == 0)
            new_seed_val = rand();

        else {
            // Trying to parse string to int
            auto [ptr, ec] = std::from_chars(
                new_seed_str.data(),
                new_seed_str.data() + new_seed_str.size(),
                new_seed_val);

            // If not possible, hashing string to int
            if (ec != std::errc() || ptr != new_seed_str.data() + new_seed_str.size()) {
                // https://stackoverflow.com/questions/16075271/hashing-a-string-to-an-integer-in-c
                std::cout << new_seed_str << " is not a number, hashing...\n";
                new_seed_val = std::hash<std::string_view>{}(new_seed_str);
            }
        }

        std::cout << new_seed_val << std::endl;
        clear();
        _world->clear();
        _world->setWorldSeed(new_seed_val);
        cleared = true;
    }

    ImGui::DragInt("floor height", &_world->m_floor_height, 1.f);
    ImGui::DragInt("mountain height", &_world->m_mountain_height, 1.f);
    ImGui::DragInt("water height", &_world->m_water_height, 1.f);

    ImGui::End();
    return cleared;
}
