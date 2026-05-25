// GLEW
#include <GL/glew.h>

// IMGUI
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

// USUAL INCLUDES
#include <stdio.h>
#include <stdlib.h>

#include <atomic>
#include <chrono>
#include <iostream>
#include <shared_mutex>
#include <string>
#include <thread>

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

constexpr size_t WORLD_TPS = 10;
constexpr float WORLD_FRAME_TIME = 1.0f / WORLD_TPS;
void worldThread() {
    float current_time, delta_time, last_frame = glfwGetTime();
    while (!kill_threads) {
        current_time = glfwGetTime();
        delta_time = current_time - last_frame;
        if (delta_time < WORLD_FRAME_TIME) {
            std::this_thread::sleep_for(std::chrono::duration<float>(0.5f * (WORLD_FRAME_TIME - delta_time)));
            continue;
        }
        last_frame = current_time;

        std::vector<glm::vec3> pos_copy;
        {
            ReadLock pos_write(pos_lock);
            pos_copy = controlled_pos;
        }

        {
            WriteLock world_write(world_lock);
            world->update(pos_copy);
        }

        {
            WriteLock renderer_write(renderer_lock);
            world_renderer.updateWorld(world.get());
        }
        // world_renderer.updateInterface(window);
    }
}

int main(void) {
    globalInit(window);
    window.toggleMouseCapture();

    world = std::make_unique<World>();
    world_renderer.setWorld(world.get(), camera);
    // world_renderer.reserve(world.get());

    // Create player entities
    ECS::EntityId truc = ecs_manager.createEntity<ECS::TestEntity>({world.get(), glm::vec3(16.5f, 100.f, 16.5f)});
    ecs_manager.getComponent<ECS::Movable>(truc).vel = glm::vec3(1.f, 0.f, 0.f);
    ecs_manager.startControl(truc);
    ECS::EntityId truc2 = ecs_manager.createEntity<ECS::TestEntity>({world.get(), glm::vec3(16.5f, 100.f, 16.5f)});
    ecs_manager.getComponent<ECS::Movable>(truc2).vel = glm::vec3(-3.f, 0.f, 0.f);
    // ecs_manager.startControl(truc2);

    // Setup key bindings
    GLenum polygon_mode{GL_FILL};
    bool display_debug{false};
    window.keyboard.bind(
        GLFW_KEY_BACKSPACE,
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
            world->update(controlled_pos);
        },
        nullptr);

    bool save_shadowmap = false;
    window.keyboard.bind(GLFW_KEY_P, [&save_shadowmap]() { save_shadowmap = true; }, nullptr);

    // Create and compile our GLSL program from the shaders
    ShaderProgram line_shader("src/shaders/line_vertex.glsl", "src/shaders/line_fragment.glsl");
    ShaderProgram chunk_shader("src/shaders/chunk_vertex.glsl", "src/shaders/pbr_fragment.glsl");
    ShaderProgram chunk_shadowmap_shader("src/shaders/chunk_shadowmap_vertex.glsl", "src/shaders/chunk_shadowmap_fragment.glsl");

    // Test de l'atlas vide, qui est utilisé comme base pour l'atlas courant
    GrayScaleTexture atlas_videGrayScale(128, 128);
    atlas_videGrayScale.savePNG("build/test_atlasGrayScale");
    RGBTexture atlas_videRGB(128, 128);
    atlas_videRGB.savePNG("build/test_atlasRGB");
    RGBATexture atlas_videRGBA(128, 128);
    atlas_videRGBA.savePNG("build/test_atlasRGBA");

    auto atlasses = RGBATexture::generateAtlasses();
    RGBATexture& albedo_atlas = atlasses[0];
    RGBATexture& normal_atlas = atlasses[1];
    RGBATexture& specular_atlas = atlasses[2];
    albedo_atlas.savePNG("build/albedo_atlas");
    normal_atlas.savePNG("build/normal_atlas");
    specular_atlas.savePNG("build/specular_atlas");
    albedo_atlas.initShaderData();
    normal_atlas.initShaderData();
    specular_atlas.initShaderData();

    ShadowMap sun_shadowmap{2048, 2048};
    sun_shadowmap.initShaderData();

    { // Pre start actions
        const std::unordered_set<ECS::EntityId>& controlled_entities = ecs_manager.getSystem<ECS::ControllingSystem>().m_entities;
        controlled_pos.clear();
        controlled_pos.reserve(controlled_entities.size());
        for (ECS::EntityId entity : controlled_entities) {
            controlled_pos.push_back(ecs_manager.getComponent<ECS::Positionnable>(entity).pos);
        }

        world->update(controlled_pos);
        camera.update(window, ecs_manager, 1.f);
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
            currentFrame = glfwGetTime();
            glfwSwapBuffers(window.getWindow());
            glfwPollEvents();
        }
        delta_time = currentFrame - last_frame;
        last_frame = currentFrame;

        glm::vec3 cam_pos;
        {
            ReadLock window_read(window_lock);
            WriteLock camera_write(camera_lock);
            ReadLock entities_read(entities_lock);
            camera.update(window, ecs_manager, delta_time);
            cam_pos = camera.getCamPos();
        }
        {
            WriteLock window_write(window_lock);
            ReadLock world_read(world_lock);
            WriteLock entities_write(entities_lock);
            ecs_manager.update(window, delta_time);
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

        if (save_shadowmap) {
            save_shadowmap = false;

            glCullFace(GL_FRONT);
            // TODO: pour chaque light
            glm::mat4 VP;
            {
                ReadLock renderer_read(renderer_lock);
                VP = world_renderer.sunVP(cam_pos);
                sun_shadowmap.bind(VP);

                chunk_shadowmap_shader.use();
                chunk_shadowmap_shader.set("VP", VP);
                chunk_shadowmap_shader.set("albedo_atlas", albedo_atlas.getGpuSlot());
                world_renderer.renderChunkShadows(chunk_shadowmap_shader);
            }
            glCullFace(GL_BACK);

            sun_shadowmap.savePNG("sun_shadowmap");
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
            ReadLock window_read(window_lock);
            glViewport(0, 0, window.getSize().x, window.getSize().y);
        }
        {
            ReadLock camera_read(camera_lock);
            WriteLock renderer_write(renderer_lock);
            world_renderer.renderChunks(chunk_shader, camera, albedo_atlas, normal_atlas, specular_atlas, sun_shadowmap);
        }
        if (display_debug) {
            ReadLock camera_read(camera_lock);
            ReadLock renderer_read(renderer_lock);
            world_renderer.renderDebugBoxes(line_shader, camera);
        }
        {
            ReadLock camera_read(camera_lock);
            ReadLock entities_read(entities_lock);
            ecs_manager.render(line_shader, camera.getView(), camera.getProjection());
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

        WriteLock window_write(window_lock);
        window.clearMovements();
    } while (glfwWindowShouldClose(window.getWindow()) == GLFW_FALSE);

    kill_threads = true;
    world_thread.join();

    world_renderer.clear();
    ecs_manager.destroyEntities();
    world->clear();

    return 0;
}