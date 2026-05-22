// USUAL INCLUDES
#include "World.hpp"

Chunk* World::findChunk(const glm::ivec3& _chunk_pos) {
    return m_chunks.at(_chunk_pos);
}

Block* World::findBlock(const glm::ivec3& _block_pos) {
    glm::ivec3 chunk_pos = Chunk::blockPosToChunkPos(_block_pos);
    Chunk* chunk = m_chunks.at(chunk_pos);
    return chunk != nullptr ? &chunk->getBlock(_block_pos)
                            : nullptr;
}

std::vector<const Block*> World::findSolidBlocks(const glm::ivec3& start, const glm::ivec3& end) {
    Chunk* start_chunk = findChunk(Chunk::blockPosToChunkPos(start));
    std::vector<const Block*> res;
    if (start_chunk != nullptr)
        start_chunk->findSolidBlocks(start, end, res);
    return res;
}

Chunk* World::addChunk(const glm::ivec3& _chunk_pos) {
    if (findChunk(_chunk_pos) != nullptr)
        return nullptr;

    m_chunks.emplace(_chunk_pos, this, _chunk_pos, m_gentype);
    m_chunks_frontier.erase(_chunk_pos);
    Chunk* inserted_chunk = m_chunks.at(_chunk_pos);

    // on ajoute tous ses voisins dans la frontière si ils ne sont pas déjà chargé sinon on update le chunk car il a un nouveau voisin !
    for (int face_i = 0; face_i < 6; face_i++) {
        glm::ivec3 neighbour_pos = _chunk_pos + Chunk::NEIGHBOURS_POS[face_i];
        Chunk* neighbour = findChunk(neighbour_pos);
        if (neighbour == nullptr) {
            m_chunks_frontier.insert(neighbour_pos);
        } else {
            inserted_chunk->m_neighbours[face_i] = neighbour;
            neighbour->m_neighbours[OPPOSITE_FACE[face_i]] = inserted_chunk;

            inserted_chunk->updateBlockNeighbours(face_i);
            if (neighbour->m_should_rebuild_mesh != nullptr)
                *neighbour->m_should_rebuild_mesh = true;
        }
    }

    m_added_chunks.push_back(inserted_chunk);
    return inserted_chunk;
}
bool World::removeChunk(const glm::ivec3& _chunk_pos) {
    Chunk* removed_chunk = findChunk(_chunk_pos);
    if (removed_chunk == nullptr)
        return false;
    m_chunks.remove(_chunk_pos);
    m_chunks_frontier.erase(_chunk_pos);

    for (int face_i = 0; face_i < 6; face_i++) {
        glm::ivec3 neighbour_pos = _chunk_pos + Chunk::NEIGHBOURS_POS[face_i];
        Chunk* neighbour = findChunk(neighbour_pos);
        if (neighbour != nullptr) {
            // neighbour chunk is loaded, we update it and put the current chunk in the frontier
            m_chunks_frontier.insert(_chunk_pos);
            neighbour->m_neighbours[OPPOSITE_FACE[face_i]] = nullptr;
            neighbour->updateBlockNeighbours(OPPOSITE_FACE[face_i]);
            if (neighbour->m_should_rebuild_mesh != nullptr)
                *neighbour->m_should_rebuild_mesh = true;
        } else if (isChunkFrontier(neighbour_pos)) {
            // neighbour chunk is in frontier, remove it if it has no loaded neighbour.
            bool neighbour_has_neighbour = false;
            for (int neighbour_face_i = 0; neighbour_face_i < 6; neighbour_face_i++) {
                glm::ivec3 neighbour_neighbour_pos = neighbour_pos + Chunk::NEIGHBOURS_POS[face_i];
                if (findChunk(neighbour_neighbour_pos) != nullptr) {
                    neighbour_has_neighbour = true;
                    break;
                }
            }
            if (!neighbour_has_neighbour) {
                m_chunks_frontier.erase(neighbour_pos);
            }
        }
    }

    m_removed_chunks.push_back(_chunk_pos);
    return true;
}

bool World::generate(const std::vector<glm::vec3>& _positions) {
    bool chunk_in_pos = false;
    for (const glm::vec3& pos : _positions) {
        glm::ivec3 _chunk_pos = Chunk::posToChunkPos(pos);
        if (findChunk(_chunk_pos) == nullptr) {
            addChunk(_chunk_pos);
            chunk_in_pos = true;
        }
    }
    if (chunk_in_pos)
        return true;

    bool removed = false;
    m_chunks.forEach([&](Chunk& chunk) {
        bool should_remove = true;
        for (const glm::vec3& pos : _positions) {
            if (Chunk::chunkDistance(chunk.getPos(), pos) <= m_render_distance) {
                should_remove = false;
                break;
            }
        }
        if (should_remove) {
            removeChunk(chunk.getPos());
            removed = true;
        }
    });
    if (removed)
        return true;

    std::vector<std::map<float, glm::ivec3>> chunk_to_add(_positions.size());
    for (const glm::ivec3& chunk_pos : m_chunks_frontier) {
        for (uint i = 0; i < _positions.size(); i++) {
            float dist = Chunk::chunkDistance(_positions[i], chunk_pos);
            chunk_to_add[i].insert({dist, chunk_pos});
        }
    }

    bool res = false;
    for (uint i = 0; i < _positions.size(); i++) {
        if (!chunk_to_add[i].empty()) {
            addChunk(chunk_to_add[i].begin()->second);
            res = true;
        }
    }

    return res;
}

bool World::updateTime() {
    if (!m_play)
        return false;

    if (m_time_ff) {
        m_last_update = glfwGetTime();
        m_time_ff = false;
        return false;
    }

    double current_time = glfwGetTime();                               // in seconds, capture
    double delta_time = current_time - m_last_update;                  // in seconds, computing delta time
    m_last_update = current_time;                                      // update for next step
    double delta_ticks = delta_time * TICK_SPEED;                      // seconds * ticks / seconds = ticks
    m_tick_accumulator += delta_ticks;                                 // save to avoir drifting
    uint64_t ticks_passed = static_cast<uint64_t>(m_tick_accumulator); // seconds -> ticks
    m_tick_accumulator -= ticks_passed;                                // reset for next delta
    m_world_time = (m_world_time + ticks_passed) % DAY_LENGTH;         // in ticks % ticks per day (time changed)
    return true;
}

void World::updateChunks() {
    // load / unload the chunks
    std::vector<glm::vec3> controlled_pos;
    std::unordered_set<ECS::EntityId>& controlled_entities = m_ecs_manager.getSystem<ECS::ControllingSystem>().m_entities;
    controlled_pos.reserve(controlled_entities.size());
    for (ECS::EntityId entity : controlled_entities)
        controlled_pos.push_back(m_ecs_manager.getComponent<ECS::Positionnable>(entity).pos);
    generate(controlled_pos);

    // update the chunks (water, redstone, ....)
}

// -------------------------------------------------------------------------
// WORLD RENDERER
// -------------------------------------------------------------------------

void WorldRenderer::setWorld(World* _world) {
    if (m_world == _world)
        return;
    m_world = _world;
    if (m_world == nullptr)
        return;

    m_world->m_added_chunks = std::list<Chunk*>();
    m_world->m_removed_chunks = std::list<glm::ivec3>();

    glm::vec3 cam_pos = m_world->m_ecs_manager.getSystem<ECS::CamerableSystem>().getCamPos();
    m_world->m_chunks.forEach([&](Chunk& chunk) {
        m_render_chunks.emplace(chunk.getPos(), &chunk, cam_pos);
    });
}
void WorldRenderer::clear() {
    if (m_world != nullptr)
        m_world = nullptr;
    m_render_chunks.clear();
}

glm::vec3 WorldRenderer::skyColor() const {
    if (m_world == nullptr)
        return glm::vec3(0.);
    double angle = m_world->m_world_time * M_PI * 2.0 / DAY_LENGTH;
    float t = (sin(angle) + 1.0f) * 0.5f;
    return glm::mix(SKY_NIGHT, SKY_DAY, t);
}
glm::vec3 WorldRenderer::sunColor() const {
    if (m_world == nullptr)
        return glm::vec3(0.);
    double angle = (m_world->m_world_time * M_PI * 4.0 / DAY_LENGTH) + M_PI_2;
    float t = (sin(angle) + 1.0f) * 0.5f;
    return glm::mix(SUN_NOON, SUN_DUSK, t);
}

void WorldRenderer::updateLoadedChunks() {
    m_removed_chunks = m_world->m_removed_chunks;
    m_added_chunks = m_world->m_added_chunks;
    m_world->m_removed_chunks.clear();
    m_world->m_added_chunks.clear();
}

void WorldRenderer::renderChunks(ShaderProgram& _chunk_shader) {
    if (m_world == nullptr)
        return;

    glm::vec2 sun_angle(m_sun_season, m_world->m_world_time * TIME_ANGLE_FACTOR); // angle du soleil (nord<->sud, est<->ouest)
    glm::vec3 sun_direction(sin(sun_angle.x), cos(sun_angle.x) * sin(sun_angle.y), cos(sun_angle.x) * cos(sun_angle.y));
    glm::vec3 sun_color = sunColor();

    const ECS::CamerableSystem& camerable_system = m_world->m_ecs_manager.getSystem<ECS::CamerableSystem>();
    const Frustum& frustum = camerable_system.getFrustum();
    const glm::vec3& cam_pos = camerable_system.getCamPos();

    for (const glm::ivec3& chunk_pos : m_removed_chunks)
        m_render_chunks.remove(chunk_pos);
    for (Chunk* chunk : m_added_chunks)
        m_render_chunks.emplace(chunk->getPos(), chunk, cam_pos);
    m_removed_chunks.clear();
    m_added_chunks.clear();

    glm::ivec3 cam_chunk = Chunk::posToChunkPos(camerable_system.getCamPos());
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
        if (chunk_renderer.getOpaqueTriangles() > 0 && chunk_renderer.getTranslucentTriangles() > 0 // on skip les chunks sans triangles
            && frustum.isVisible(chunk_renderer.getAABB())                                          // On skip les chunk hors du frustum
        ) {
            float dist = glm::distance(cam_pos, glm::vec3(chunk_renderer.getPos()) + glm::vec3(Chunk::CHUNK_SIZE * 0.5f));
            draw_list.push_back({dist, &chunk_renderer});
        }
    });
    std::sort(draw_list.begin(), draw_list.end(),
              [](auto& a, auto& b) { return a.first > b.first; });

    // --- PASS 3: draw (sequential pointer chasing, unavoidable) ---
    _chunk_shader.use();
    _chunk_shader.set("view", camerable_system.getView());
    _chunk_shader.set("projection", camerable_system.getProjection());
    _chunk_shader.set("camera_pos", cam_pos);
    _chunk_shader.set("sun_direction", sun_direction);
    _chunk_shader.set("sun_color", sun_color);
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
void WorldRenderer::renderDebugBoxes(ShaderProgram& _line_shader) const {
    if (m_world == nullptr)
        return;
    const ECS::CamerableSystem& camerable_system = m_world->m_ecs_manager.getSystem<ECS::CamerableSystem>();
    _line_shader.use();
    _line_shader.set("view", camerable_system.getView());
    _line_shader.set("projection", camerable_system.getProjection());
    _line_shader.set("color", glm::vec3(1.f));

    AABB<float> chunk_hitbox(glm::vec3(0), glm::vec3(Chunk::CHUNK_SIZE));
    AABBRenderer box(chunk_hitbox);
    m_render_chunks.forEach([&_line_shader, &box](const ChunkRenderer& chunk_renderer) {
        _line_shader.set("position", glm::vec3(chunk_renderer.getPos()));
        box.render();
    });
    box.clearShaderData();

    chunk_hitbox.min = glm::vec3(Chunk::CHUNK_SIZE / 4);
    chunk_hitbox.max = glm::vec3(3 * Chunk::CHUNK_SIZE / 4);
    box.initShaderData(chunk_hitbox);
    for (const glm::ivec3& chunk_pos : m_world->m_chunks_frontier) {
        _line_shader.set("position", glm::vec3(chunk_pos));
        box.render();
    }
    box.clearShaderData();
}
void WorldRenderer::renderEntities(ShaderProgram& _line_shader) const {
    if (m_world != nullptr)
        m_world->m_ecs_manager.render(_line_shader);
}

void WorldRenderer::updateInterface(Window& _window) {
    if (m_world == nullptr || !ImGui::Begin("World Info")) {
        ImGui::End();
        return;
    }

    m_world->m_ecs_manager.updateInterfaces(_window);

    int current_type = static_cast<int>(m_world->m_gentype);
    if (ImGui::Combo("Generation Type", &current_type, GenTypeNames, IM_ARRAYSIZE(GenTypeNames))) {
        m_world->m_gentype = static_cast<GenType>(current_type);
        m_render_chunks.clear();
        m_world->clearChunks();
        m_world->updateChunks();
    }

    if (ImGui::InputInt("Render distance", &m_world->m_render_distance, 1, 2)) {
        m_world->m_render_distance = std::max(m_world->m_render_distance, 1);
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    if (ImGui::DragScalar("World Time", ImGuiDataType_U64, &m_world->m_world_time, 10.0f, 0, &DAY_LENGTH)) {
        m_world->updateTime();
    }
    if (ImGui::DragFloat("World Season", &m_sun_season, 0.01f, -M_2_PIf, M_2_PIf)) {
        m_sun_season = std::clamp(m_sun_season, -M_2_PIf, M_2_PIf);
    }
    if (ImGui::Button(m_world->m_play ? "Pause" : "Play")) {
        m_world->toggleTime();
    }
    ImGui::End();
}
