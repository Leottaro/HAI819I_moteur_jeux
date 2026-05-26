// GLEW
#include <GL/glew.h>

// IMGUI
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

// FILEWATCH
#include <FileWatch.hpp>

// USUAL INCLUDES
#include <stdio.h>
#include <stdlib.h>

#include <atomic>
#include <chrono>
#include <iostream>
#include <shared_mutex>
#include <string>
#include <thread>
#include <utility>

#include "Camera.hpp"
#include "Chunk.hpp"
#include "ECS/ECS.hpp"
#include "GLGlobalContext.hpp"
#include "ShaderProgram.hpp"
#include "Texture.hpp"
#include "Window.hpp"
#include "WorldRenderer.hpp"

using namespace std;

void initOpenGL() {
    glClearColor(0.1f, 0.1f, 0.3f, 0.0f);              // Dark blue background
    glEnable(GL_DEPTH_TEST);                           // Enable depth test
    glDepthFunc(GL_LESS);                              // Accept fragment if it closer to the camera than the former one
    glEnable(GL_BLEND);                                // Enable color blending (for alpha)
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // Set a blending function
    glEnable(GL_CULL_FACE);                            // Cull triangles which normal is not towards the camera
}

void globalInit(Window& window) {
#if defined(__linux__)
    glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_X11);
#endif
    // INITIALIZE GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW!" << std::endl;
        exit(EXIT_FAILURE);
    }
    window.init();

    // INITIALIZE GLEW
    GLenum err = glewInit();
    if (err != GLEW_OK && err != 4) {
        /* Problem: glewInit failed, something is seriously wrong. */
        fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
        exit(EXIT_FAILURE);
    }

    // INITIALIZE IMGUI
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    // ImGuiIO &io = ImGui::GetIO();
    // io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    // io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls
    // io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;     // IF using Docking Branch
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window.getWindow(), true); // Second param install_callback=true will install GLFW callbacks and chain to existing ones.
    ImGui_ImplOpenGL3_Init();

    initOpenGL();
}

Window window;
Camera camera;
std::unique_ptr<World> world;
WorldRenderer world_renderer;
ECSManager ecs_manager;
GLGlobalContext gl_global_context;

// Create and compile our GLSL program from the shaders
ShaderProgram line_shader("src/shaders/line_vertex.glsl", "src/shaders/line_fragment.glsl");
ShaderProgram chunk_shader("src/shaders/chunk_vertex.glsl", "src/shaders/pbr_fragment.glsl");
ShaderProgram chunk_shadow_shader("src/shaders/chunk_shadow_vertex.glsl", "src/shaders/chunk_shadow_fragment.glsl");

RGBATexture2DArray albedo_textures(0, 0, TEXTURE_NUMBER);
RGBATexture2DArray normal_textures(0, 0, TEXTURE_NUMBER);
RGBATexture2DArray specular_textures(0, 0, TEXTURE_NUMBER);

// https://stackoverflow.com/questions/244316/reader-writer-locks-in-c
typedef std::shared_mutex Lock;
typedef std::unique_lock<Lock> WriteLock;
typedef std::shared_lock<Lock> ReadLock;

std::vector<glm::vec3> controlled_pos{};
std::atomic<bool> kill_threads{false};
Lock window_lock;
Lock camera_lock;
Lock world_lock;
Lock renderer_lock;
Lock entities_lock;
Lock pos_lock;

constexpr size_t WORLD_TPS = 20;
constexpr float WORLD_FRAME_TIME = 1.0f / WORLD_TPS;
void worldThread() {
    float current_time, delta_time, world_time = glfwGetTime();
    while (!kill_threads) {
        current_time = glfwGetTime();
        delta_time = current_time - world_time;
        if (delta_time < WORLD_FRAME_TIME) {
            std::this_thread::sleep_for(std::chrono::duration<float>(0.5f * (WORLD_FRAME_TIME - delta_time)));
            continue;
        }
        world_time += WORLD_FRAME_TIME;

        std::vector<glm::vec3> pos_copy;
        {
            ReadLock pos_write(pos_lock);
            pos_copy = controlled_pos;
        }
        {
            WriteLock world_write(world_lock);
            ReadLock renderer_read(renderer_lock);
            ReadLock entities_read(entities_lock);
            ecs_manager.updateWorld(world.get());
        }
        {
            WriteLock entities_write(entities_lock);
            ecs_manager.clearWorldUpdates();
        }
        {
            WriteLock world_write(world_lock);
            world->selfUpdate(pos_copy);
        }
        {
            WriteLock renderer_write(renderer_lock);
            world_renderer.updateWorld(world.get());
        }
    }
}

void shaderReloadCallback() {
    line_shader.load();
    chunk_shader.load();
    chunk_shadow_shader.load();
}

void textureReloadCallback() {
    RGBATexture2DArray::generateTextureArrays(albedo_textures, normal_textures, specular_textures);
}

template <class OnChange>
auto makeTextureWatcher(const std::string& path, OnChange&& onChange) {
    return filewatch::FileWatch<std::string>(
        path,
        [onChange = std::forward<OnChange>(onChange)](const std::string& changed_path, const filewatch::Event change_type) {
            onChange(changed_path, change_type);
        });
}

int main(void) {
    globalInit(window);
    // window.toggleMouseCapture();

    world = std::make_unique<World>();
    world_renderer.setWorld(world.get(), camera);
    // world_renderer.reserve(world.get());

    // Optionnel, pour showcase les features
    RGBATexture albedo_atlas, normal_atlas, specular_atlas;
    RGBATexture::generateAtlasses(albedo_atlas, normal_atlas, specular_atlas);
    albedo_atlas.savePNG("build/albedo_atlas");
    normal_atlas.savePNG("build/normal_atlas");
    specular_atlas.savePNG("build/specular_atlas");

    // Setup key bindings
    GLenum polygon_mode{GL_FILL};
    bool display_debug{false};
    window.keyboard.bind(
        GLFW_KEY_K,
        [&]() {
            std::lock_guard<std::mutex> lock(Window::glfw_mutex);
            glfwSetWindowShouldClose(window.getWindow(), GLFW_TRUE);
        },
        nullptr);
    window.keyboard.bind(GLFW_KEY_ESCAPE, [&]() { window.toggleMouseCapture(); }, nullptr);
    window.keyboard.bind(GLFW_KEY_F11, [&]() { window.setFullscreen(!window.getFullscreen()); }, nullptr);
    window.keyboard.bind(GLFW_KEY_Z, [&]() {
        if (polygon_mode == GL_FILL) {
            polygon_mode = GL_LINE;
            } else if (polygon_mode == GL_LINE) {
                polygon_mode = GL_POINT;
            } else if (polygon_mode == GL_POINT) {
                polygon_mode = GL_FILL;
            }
            glPolygonMode(GL_FRONT_AND_BACK, polygon_mode); }, nullptr);
    window.keyboard.bind(GLFW_KEY_G, [&]() { display_debug = !display_debug; }, nullptr);
    window.keyboard.bind(
        GLFW_KEY_C,
        [&]() {
            WriteLock entities_write(entities_lock);
            ecs_manager.toggleControlType();
        },
        nullptr);
    window.keyboard.bind(
        GLFW_KEY_R,
        [&]() {
            WriteLock world_write(world_lock);
            WriteLock renderer_write(renderer_lock);
            ReadLock pos_read(pos_lock);

            world_renderer.clear();
            world->clear();
            world->selfUpdate(controlled_pos);

            shaderReloadCallback();
        },
        nullptr);

    bool save_shadowmap = false;
    window.keyboard.bind(GLFW_KEY_P, [&save_shadowmap]() { save_shadowmap = true; }, nullptr);

    shaderReloadCallback();

    std::atomic<bool> shader_reload_requested{false};
    filewatch::FileWatch<std::string> shaderWatcher(
        "src/shaders/",
        [&shader_reload_requested](const std::string& path, const filewatch::Event change_type) {
            static std::string last_path;
            static filewatch::Event last_change_type{};
            static auto last_event_time = std::chrono::steady_clock::time_point::min();

            const auto now = std::chrono::steady_clock::now();
            const auto within_debounce = (now - last_event_time) < std::chrono::milliseconds(150);
            if (within_debounce && path == last_path && change_type == last_change_type) {
                return;
            }

            last_path = path;
            last_change_type = change_type;
            last_event_time = now;

            std::cout << "Shader modified: " << path << ". Reloading shaders... \n";
            shader_reload_requested.store(true, std::memory_order_release);
        });

    textureReloadCallback();

    std::atomic<bool> texture_reload_requested{false};
    auto texture_change_handler = [&texture_reload_requested](const std::string& path, const filewatch::Event change_type) {
        static std::string last_path;
        static filewatch::Event last_change_type{};
        static auto last_event_time = std::chrono::steady_clock::time_point::min();

        const auto now = std::chrono::steady_clock::now();
        const auto within_debounce = (now - last_event_time) < std::chrono::milliseconds(150);
        if (within_debounce && path == last_path && change_type == last_change_type) {
            return;
        }

        last_path = path;
        last_change_type = change_type;
        last_event_time = now;

        std::cout << "Texture modifie: " << path << ". Reloading textures... \n";
        texture_reload_requested.store(true, std::memory_order_release);
    };

    auto albedoWatcher = makeTextureWatcher("ressources/textures/albedos", texture_change_handler);
    auto normalWatcher = makeTextureWatcher("ressources/textures/normals", texture_change_handler);
    auto specularWatcher = makeTextureWatcher("ressources/textures/speculars", texture_change_handler);

    ShadowMap sun_shadowmap{2048, 2048};
    sun_shadowmap.initShaderData();

    { // Pre start actions
        // Create player entities
        ECS::EntityId truc = ecs_manager.createEntity<ECS::TestEntity>({glm::vec3(16.5f, 100.f, 16.5f)});
        ecs_manager.getComponent<ECS::Movable>(truc).vel = glm::vec3(1.f, 0.f, 0.f);
        ecs_manager.startControl(truc);
        ECS::EntityId truc2 = ecs_manager.createEntity<ECS::TestEntity>({glm::vec3(16.5f, 100.f, 16.5f)});
        ecs_manager.getComponent<ECS::Movable>(truc2).vel = glm::vec3(-3.f, 0.f, 0.f);

        const std::unordered_set<ECS::EntityId>& controlled_entities = ecs_manager.getSystem<ECS::ControllingSystem>().m_entities;
        controlled_pos.clear();
        controlled_pos.reserve(controlled_entities.size());
        for (ECS::EntityId entity : controlled_entities) {
            controlled_pos.push_back(ecs_manager.getComponent<ECS::Positionnable>(entity).pos);
        }

        world->selfUpdate(controlled_pos);
        camera.update(window, world.get(), ecs_manager, 1.f);
    }

    // Start world thread
    thread world_thread(worldThread);

    float currentFrame, delta_time, last_frame = glfwGetTime();
    do {
        gl_global_context.flush();

        // ----------------------------------------------------------------------------------------------------
        // ----------------------------------------   OBJECTS UPDATES   ---------------------------------------
        // ----------------------------------------------------------------------------------------------------

        {
            WriteLock window_write(window_lock);
            window.clearMovements();
            currentFrame = glfwGetTime();
            glfwSwapBuffers(window.getWindow());
            glfwPollEvents();
        }

        if (shader_reload_requested.exchange(false, std::memory_order_acq_rel)) {
            shaderReloadCallback();
        }

        if (texture_reload_requested.exchange(false, std::memory_order_acq_rel)) {
            textureReloadCallback();
        }

        delta_time = currentFrame - last_frame;
        last_frame = currentFrame;
        {
            WriteLock window_write(window_lock);
            ReadLock world_read(world_lock);
            WriteLock entities_write(entities_lock);
            ecs_manager.update(window, world.get(), delta_time);
        }
        glm::vec3 cam_pos;
        glm::mat4 cam_proj, cam_view;
        Camera::Frustum cam_frustum;
        {
            ReadLock window_read(window_lock);
            WriteLock camera_write(camera_lock);
            ReadLock world_read(world_lock);
            ReadLock entities_read(entities_lock);
            camera.update(window, world.get(), ecs_manager, delta_time);
            cam_pos = camera.getCamPos();
            cam_proj = camera.getProjection();
            cam_view = camera.getView();
            cam_frustum = camera.getFrustum();
        }

        std::vector<glm::vec3> pos_copy;
        {
            ReadLock entities_read(entities_lock);
            ReadLock pos_write(pos_lock);

            std::unordered_set<ECS::EntityId> controlled_entities = ecs_manager.getSystem<ECS::ControllingSystem>().m_entities;
            controlled_pos.clear();
            controlled_pos.reserve(controlled_entities.size());
            pos_copy.reserve(controlled_entities.size());
            for (ECS::EntityId entity : controlled_entities) {
                controlled_pos.push_back(ecs_manager.getComponent<ECS::Positionnable>(entity).pos);
                pos_copy.push_back(ecs_manager.getComponent<ECS::Positionnable>(entity).pos);
            }
        }

        // ----------------------------------------------------------------------------------------------------
        // --------------------------------------   SHADOWMAP RENDERING   -------------------------------------
        // ----------------------------------------------------------------------------------------------------

        // glCullFace(GL_FRONT);
        { // TODO: pour chaque light
            glm::mat4 VP;
            {
                ReadLock renderer_read(renderer_lock);
                VP = world_renderer.sunVP(cam_pos);
                sun_shadowmap.bind(VP);
            }

            // TODO: pour chaque objets
            {
                ReadLock renderer_read(renderer_lock);
                chunk_shadow_shader.use();
                chunk_shadow_shader.set("VP", VP);
                chunk_shadow_shader.set("albedo_textures", albedo_textures.getGpuSlot());
                world_renderer.renderChunkShadows(chunk_shadow_shader);
            }

            if (save_shadowmap) {
                save_shadowmap = false;
                sun_shadowmap.savePNG("sun_shadowmap");
            }
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        // glCullFace(GL_BACK);
        {
            ReadLock window_read(window_lock);
            glViewport(0, 0, window.getEffectiveSize().x, window.getEffectiveSize().y);
        }

        // ----------------------------------------------------------------------------------------------------
        // ----------------------------------------   FINAL RENDERING   ---------------------------------------
        // ----------------------------------------------------------------------------------------------------

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        {
            ReadLock renderer_read(renderer_lock);
            glClearColor(world_renderer.m_sky_color.r, world_renderer.m_sky_color.g, world_renderer.m_sky_color.b, 1.f);
        }
        {
            WriteLock renderer_write(renderer_lock);
            chunk_shader.use();
            chunk_shader.set("view", cam_view);
            chunk_shader.set("projection", cam_proj);
            chunk_shader.set("camera_pos", cam_pos);
            chunk_shader.set("sun_direction", world_renderer.getSunDirection());
            chunk_shader.set("sun_color", world_renderer.getSunColor());
            chunk_shader.set("sun_shadowmap", sun_shadowmap.getGpuSlot());
            chunk_shader.set("sun_VP", sun_shadowmap.getVP());
            chunk_shader.set("albedo_textures", albedo_textures.getGpuSlot());
            chunk_shader.set("normal_textures", normal_textures.getGpuSlot());
            chunk_shader.set("specular_textures", specular_textures.getGpuSlot());
            world_renderer.renderChunks(chunk_shader, cam_frustum, cam_pos);
        }

        // Line rendering
        line_shader.use();
        line_shader.set("view", cam_view);
        line_shader.set("projection", cam_proj);
        line_shader.set("color", glm::vec3(1.f));

        if (display_debug) {
            ReadLock renderer_read(renderer_lock);
            line_shader.set("color", glm::vec3(1.f));
            world_renderer.renderDebugBoxes(line_shader);
        }
        {
            ReadLock entities_read(entities_lock);
            ecs_manager.render(line_shader);
        }

        // ----------------------------------------------------------------------------------------------------
        // ----------------------------------------   IMGUI RENDERING   ---------------------------------------
        // ----------------------------------------------------------------------------------------------------

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        {
            WriteLock camera_write(camera_lock);
            WriteLock entities_write(entities_lock);
            camera.updateInterface(ecs_manager);
        }
        {
            WriteLock world_write(world_lock);
            WriteLock renderer_write(renderer_lock);
            world_renderer.updateInterface(world.get(), pos_copy);
        }
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    } while (glfwWindowShouldClose(window.getWindow()) == GLFW_FALSE);

    kill_threads = true;
    world_thread.join();

    ecs_manager.destroyEntities();
    world_renderer.clear();
    world->clear();

    return 0;
}