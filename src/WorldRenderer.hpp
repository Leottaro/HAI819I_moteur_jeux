#pragma once
#include "World.hpp"
#include "Camera.hpp"
#include "Texture.hpp"

// DISPLAY CONST
constexpr glm::vec4 SKY_DAY(187.f / 255.f, 255.f / 255.f, 250.f / 255.f, 255.f / 255.f);
constexpr glm::vec4 SKY_NIGHT(14.f / 255.f, 5.f / 255.f, 61.f / 255.f, 255.f / 255.f);
constexpr glm::vec3 SUN_NOON(209.f / 255.f, 209.f / 255.f, 175.f / 255.f);
// constexpr glm::vec3 SUN_NOON(0.f / 255.f, 0.f / 255.f, 175.f / 255.f);
constexpr glm::vec3 SUN_DUSK(255.f / 255.f, 167.f / 255.f, 41.f / 255.f);

class WorldRenderer {
    // WORLD
    std::list<Chunk*> m_added_chunks;
    std::list<glm::ivec3> m_removed_chunks;
    MathHelpers::VecSet<int, 3> m_last_frontier;

    ChunkRendererStorage m_render_chunks{};
    glm::ivec3 m_last_cam_chunk{Chunk::CHUNK_SIZE + 1}; // Initialisé a un chunk impossible
    float m_sun_season{0.f};
    glm::vec3 m_sun_direction{0.f};

    uint64_t m_last_time{0};
    uint m_last_render_distance{0};

public:
    glm::vec3 m_sky_color{0.f}, m_sun_color{0.f};

    WorldRenderer(WorldRenderer&& other) = delete;
    WorldRenderer& operator=(WorldRenderer&& other) = delete;
    WorldRenderer(const WorldRenderer& other) = delete;
    WorldRenderer& operator=(const WorldRenderer& other) = delete;
    ~WorldRenderer() { m_render_chunks.clear(); }

    WorldRenderer() {}
    WorldRenderer(World* _world, const Camera& camera) { setWorld(_world, camera); }
    inline void reserve(World* _world) {
        _world->reserveChunks();
        m_render_chunks.reserve(World::getMaxChunkNumber(_world->m_render_distance));
    }

    void clear();
    void setWorld(World* _world, const Camera& camera); // need world write
    void updateWorld(World* _world);                    // need world read

    void renderChunks(ShaderProgram& _chunk_shader, const Camera& camera, const RGBATexture& albedo_atlas, const RGBATexture& normal_atlas, const RGBATexture& specular_atlas, const ShadowMap& _sun_shadowmap);
    void renderDebugBoxes(ShaderProgram& _line_shader, const Camera& camera) const;

    void updateInterface(World* _world, const std::vector<glm::vec3>& world_gen_pos);

    inline glm::mat4 sunVP(const glm::vec3& _cam_pos) const {
        // float frustum_half_size = m_last_render_distance * Chunk::CHUNK_SIZE;
        float frustum_half_size = 32.f;
        glm::vec3 light_pos = _cam_pos - (-m_sun_direction) * frustum_half_size;
        glm::mat4 view = glm::lookAt(light_pos, _cam_pos, glm::vec3(1.f, 0.f, 0.f));
        glm::mat4 proj = glm::ortho(-frustum_half_size, frustum_half_size, -frustum_half_size, frustum_half_size, 0.1f, 2.f * frustum_half_size);
        return proj * view;
    }
    inline void renderChunkShadows(ShaderProgram& _shadowmap_shader) const {
        m_render_chunks.forEach([&_shadowmap_shader](const ChunkRenderer& chunk_renderer) {
            if ((chunk_renderer.getOpaqueTriangles() > 0 || chunk_renderer.getTranslucentTriangles() > 0)) { // on skip les chunks sans triangles
                _shadowmap_shader.set("chunk_pos", chunk_renderer.getPos());
                chunk_renderer.renderOpaque();
                chunk_renderer.renderTranslucent();
            }
        });
    }
};