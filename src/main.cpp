// GLEW
#include <GL/glew.h>

// GLM
#include <glm/ext.hpp>
#include <glm/glm.hpp>
// #define GLM_ENABLE_EXPERIMENTAL
// #include <glm/gtx/string_cast.hpp>

// IMGUI
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

// USUAL INCLUDES
#include <stdio.h>
#include <stdlib.h>

#include <fstream>
#include <iostream>
#include <string>

#include "World.hpp"
#include "ECS/ECS.hpp"
#include "Chunk.hpp"
#include "Texture.hpp"
#include "Window.hpp"
#include "ShaderProgram.hpp"

using namespace std;

void initOpenGL() {
    glClearColor(0.1f, 0.1f, 0.3f, 0.0f);              // Dark blue background
    glEnable(GL_DEPTH_TEST);                           // Enable depth test
    glEnable(GL_BLEND);                                // Enable color blending (for alpha)
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // Set a blending function
    glDepthFunc(GL_LESS);                              // Accept fragment if it closer to the camera than the former one
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

int main(void) {
    Window window;
    globalInit(window);

    World world;

    GLenum polygon_mode = GL_FILL;
    bool display_debug = false;

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
    window.keyboard.bind(GLFW_KEY_R, [&]() { world.clear();    world.generate(); }, nullptr);
    window.keyboard.bind(GLFW_KEY_G, [&]() { display_debug = !display_debug; }, nullptr);

    // Create and compile our GLSL program from the shaders
    ShaderProgram line_shader("src/shaders/line_vertex.glsl", "src/shaders/line_fragment.glsl");
    ShaderProgram block_shader("src/shaders/block_vertex.glsl", "src/shaders/block_fragment.glsl");

    auto [albedo_atlas, normal_atlas, specular_atlas] = Texture::generateAtlasses();

    albedo_atlas.savePNG("ressources/textures/atlasses/albedo_atlas");
    normal_atlas.savePNG("ressources/textures/atlasses/normal_atlas");
    specular_atlas.savePNG("ressources/textures/atlasses/specular_atlas");

    // Import needed textures
    // Texture albedo_atlas("ressources/textures/albedo_atlas.png");
    albedo_atlas.initShaderData();
    // Texture normal_atlas("ressources/textures/normal_atlas.png");
    normal_atlas.initShaderData();
    // Texture specular_atlas("ressources/textures/specular_atlas.png");
    specular_atlas.initShaderData();

    ECS::EntityId truc = world.addTestEntity(glm::vec3(23.5f, 16.f, 25.5f));
    world.getEntityComponent<ECS::Movable>(truc).vel = glm::vec3(1.f, -0.5f, 0.f);
    world.startControl(window, truc);

    do {
        glfwSwapBuffers(window.getWindow());
        glfwPollEvents();

        // Imgui
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        /**********==========OBJECTS UPDATE==========**********/
        // different world thread
        world.generate();
        world.updateEntities(window);

        /**********==========RENDERING==========**********/
        block_shader.use();
        albedo_atlas.bind(0);
        normal_atlas.bind(1);
        specular_atlas.bind(2);

        world.renderChunks(block_shader);
        world.renderEntities(line_shader);
        world.updateWindow();
        if (display_debug) {
            world.renderDebugBoxes(line_shader);
        }

        // ImGui Render
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        window.clearMovements();
    } while (glfwWindowShouldClose(window.getWindow()) == GLFW_FALSE);

    return 0;
}