// GLEW
#include <GL/glew.h>

// IMGUI
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

// USUAL INCLUDES
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string>
#include <thread>
#include <mutex>
#include <shared_mutex>

#include "World.hpp"
#include "Chunk.hpp"
#include "Texture.hpp"
#include "Window.hpp"
#include "ShaderProgram.hpp"

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
GLenum polygon_mode = GL_FILL;
bool display_debug = false;

std::unique_ptr<World> world; // heap allocated
WorldRenderer world_renderer;

// https://stackoverflow.com/questions/244316/reader-writer-locks-in-c
typedef std::shared_mutex Lock;
typedef std::unique_lock<Lock> WriteLock; // C++ 11
typedef std::shared_lock<Lock> ReadLock;  // C++ 14
Lock world_lock;
bool kill_threads = false;

constexpr size_t WORLD_TPS = 10;
constexpr size_t ENTITIES_TPS = 20;

void worldThread() {
    float last_frame = 0.0f;
    float frame_time = 1.0f / WORLD_TPS;
    while (!kill_threads) {
        float current_time = glfwGetTime();
        float delta_time = current_time - last_frame;

        if (delta_time >= frame_time) {
            WriteLock w_lock(world_lock);
            world->updateTime();
            world->updateChunks();
            // world_renderer.updateInterface(window);
            world_renderer.updateLoadedChunks();
            last_frame = current_time;
        }
    }
}

void entitiesThread() { // TODO: external ecs manager ?
    float last_frame = 0.0f;
    float frame_time = 1.0f / ENTITIES_TPS;
    while (!kill_threads) {
        float current_time = glfwGetTime();
        float delta_time = current_time - last_frame;

        if (delta_time >= frame_time) {
            WriteLock w_lock(world_lock);
            world->updateEntities(window, delta_time);
            last_frame = current_time;
        }
    }
}

int main(void) {
    globalInit(window);

    world = std::make_unique<World>();
    world_renderer.setWorld(world.get());
    glm::vec3 sky_color = world_renderer.skyColor();

    // Create player entities
    ECS::EntityId truc = world->addTestEntity(glm::vec3(23.5f, 16.f, 26.5f));
    world->getEntityComponent<ECS::Movable>(truc).vel = glm::vec3(1.f, -0.5f, 0.f);
    world->startControl(window, truc);
    ECS::EntityId truc2 = world->addTestEntity(glm::vec3(23.5f, 16.f, 26.5f));
    world->getEntityComponent<ECS::Movable>(truc2).vel = glm::vec3(-3.f, -0.5f, 0.f);
    // world->startControl(window, truc2);

    // Start world thread
    thread world_thread(worldThread);
    thread entities_thread(entitiesThread);

    // Setup key bindings
    window.keyboard.bind(GLFW_KEY_ESCAPE, [&]() { glfwSetWindowShouldClose(window.getWindow(), GLFW_TRUE); }, nullptr);
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
    window.keyboard.bind(GLFW_KEY_R, [&]() { world_renderer.clear(); world->clearChunks();  world_renderer.setWorld(world.get());  world->updateChunks(); }, nullptr);
    window.keyboard.bind(GLFW_KEY_G, [&]() { display_debug = !display_debug; }, nullptr);

    // Create and compile our GLSL program from the shaders
    ShaderProgram line_shader("src/shaders/line_vertex.glsl", "src/shaders/line_fragment.glsl");
    ShaderProgram chunk_shader("src/shaders/chunk_vertex.glsl", "src/shaders/pbr_fragment.glsl");

    auto [albedo_atlas, normal_atlas, specular_atlas] = Texture::generateAtlasses();
    albedo_atlas.savePNG("ressources/textures/atlasses/albedo_atlas");
    normal_atlas.savePNG("ressources/textures/atlasses/normal_atlas");
    specular_atlas.savePNG("ressources/textures/atlasses/specular_atlas");
    albedo_atlas.initShaderData();
    normal_atlas.initShaderData();
    specular_atlas.initShaderData();

    do {
        glfwSwapBuffers(window.getWindow());
        glfwPollEvents();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glClearColor(sky_color.r, sky_color.g, sky_color.b, 255.f);

        // Imgui
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        chunk_shader.use();
        albedo_atlas.bind(0);
        normal_atlas.bind(1);
        specular_atlas.bind(2);

        // Read needed
        {
            ReadLock r_lock(world_lock);
            world_renderer.renderChunks(chunk_shader);
            world_renderer.renderEntities(line_shader);
            if (display_debug) {
                world_renderer.renderDebugBoxes(line_shader);
            }
            sky_color = world_renderer.skyColor();
        }

        // ImGui Render
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        window.clearMovements();
    } while (glfwWindowShouldClose(window.getWindow()) == GLFW_FALSE);

    kill_threads = true;
    world_thread.join();
    entities_thread.join();

    return 0;
}