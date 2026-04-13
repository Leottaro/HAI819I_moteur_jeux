// GLEW
#include <GL/glew.h>

// GLM
#include <glm/glm.hpp>
#include <glm/ext.hpp>

// GLFW
#include <GLFW/glfw3.h>
GLFWwindow *window;

// IMGUI
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

// USUAL INCLUDES
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <fstream>
#include "ShaderProgram.hpp"
#include "Camera.hpp"
#include "RigidBody.hpp"
#include "Chunk.hpp"
#include "World.hpp"

using namespace std;

// window state
GLuint window_width = 800, window_height = 600;
double window_aspect_ratio = double(window_width) / window_height;
GLenum polygon_mode = GL_FILL;
glm::vec2 cursor_pos{0, 0};
glm::vec2 cursor_vel{0, 0};
glm::vec2 scroll{0, 0};

bool run_simulation = false;
float deltaTime = 0.f;
float lastFrame = 0.f;

Camera camera;
World world;
bool display_debug = false;

void globalInit();

int main(void) {
    globalInit();

    // Create and compile our GLSL program from the shaders
    ShaderProgram shader("ressources/shaders/vertex_shader.glsl", "ressources/shaders/fragment_shader.glsl");

    // Import needed textures
    ImageBase block_atlas("ressources/textures/block_atlas.ppm");
    block_atlas.initShaderData(0);

    camera.m_type = Camera::Type::Free;
    camera.m_position = glm::vec3(16.f, 16.f, 16.f);
    camera.m_translation_speed = 32.f;
    camera.m_orientation = glm::vec2(0.f, 0.f);
    camera.m_rotation_speed = 1.f;

    world.generate(camera.m_position);

    glfwSwapInterval(1); // VSync - avoid having 3000 fps
    do {
        glfwSwapBuffers(window);
        glfwPollEvents();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Imgui
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        /**********==========CAMERA UPDATE==========**********/
        camera.update(window, deltaTime, cursor_vel, scroll);

        /**********==========OBJECTS UPDATE==========**********/
        if (run_simulation) {
            world.generate(camera.m_position);
            // run_simulation = false;
        }

        /**********==========RENDERING==========**********/
        shader.use();

        shader.set("view", camera.getViewMatrix());
        shader.set("projection", camera.getProjectionMatrix());

        world.render(shader);
        if (display_debug)
            world.renderDebugBoxes(shader);

        // ImGui Render
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Reset some controls
        scroll = glm::vec2(0.);
        cursor_vel = glm::vec2(0.);
    } while (glfwWindowShouldClose(window) == GLFW_FALSE);

    // Cleanup VBO and shader
    world.clear();
    shader.~ShaderProgram();

    // Close OpenGL window and terminate GLFW
    glfwTerminate();

    return 0;
}

void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    // cout << "framebuffer size: " << width << ", " << height << endl;
    window_width = width;
    window_height = height;
    window_aspect_ratio = double(window_width) / window_height;
    glViewport(0, 0, width, height);
}

bool w_key_pressed = false;
bool p_key_pressed = false;
bool r_key_pressed = false;
bool g_key_pressed = false;
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    // cout << "key:" << key << " scancode:" << scancode << " action:" << action << " mods:" << mods << endl;
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }

    if (key == GLFW_KEY_Z && action == GLFW_PRESS) {
        if (!w_key_pressed) {
            if (polygon_mode == GL_FILL) {
                polygon_mode = GL_LINE;
            } else if (polygon_mode == GL_LINE) {
                polygon_mode = GL_POINT;
            } else if (polygon_mode == GL_POINT) {
                polygon_mode = GL_FILL;
            }
            glPolygonMode(GL_FRONT_AND_BACK, polygon_mode);
            w_key_pressed = true;
        }
    } else {
        if (w_key_pressed) {
            w_key_pressed = false;
        }
    }

    if (key == GLFW_KEY_P && action == GLFW_PRESS) {
        if (!p_key_pressed) {
            p_key_pressed = true;
            run_simulation = !run_simulation;
        }
    } else {
        if (p_key_pressed) {
            p_key_pressed = false;
        }
    }

    if (key == GLFW_KEY_R && action == GLFW_PRESS) {
        if (!r_key_pressed) {
            r_key_pressed = true;
            world.clear();
            world.generate(camera.m_position);
        }
    } else {
        if (r_key_pressed) {
            r_key_pressed = false;
        }
    }

    if (key == GLFW_KEY_G && action == GLFW_PRESS) {
        if (!g_key_pressed) {
            g_key_pressed = true;
            display_debug = !display_debug;
        }
    } else {
        if (g_key_pressed) {
            g_key_pressed = false;
        }
    }
}

void mouse_button_callback(GLFWwindow *window, int button, int action, int mods) {
    // cout << "mouse button:" << button << " action:" << action << " mods:" << mods << endl;
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        glfwSetInputMode(window, GLFW_CURSOR, action == GLFW_PRESS ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
    }
}

void cursor_pos_callback(GLFWwindow *window, double xpos, double ypos) {
    cursor_vel.x = xpos - cursor_pos.x;
    cursor_vel.y = ypos - cursor_pos.y;
    cursor_pos.x = xpos;
    cursor_pos.y = ypos;
    // cout << "cursor_pos: (" << cursor_pos.x << ", " << cursor_pos.y << ")\tcursor_vel: (" << cursor_vel.x << ", " << cursor_vel.y << ")" << endl;
}

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
    // cout << "scroll: (" << xoffset << ", " << yoffset << ")" << endl;
    scroll.x = xoffset;
    scroll.y = yoffset;
}

void initWindow() {
    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SCALE_FRAMEBUFFER, GL_FALSE); // https://discourse.glfw.org/t/resizing-window-results-in-wrong-aspect-ratio/1268s
    glfwWindowHint(GLFW_DEPTH_BITS, 24);

    window = glfwCreateWindow(window_width, window_height, "Moteur de jeux", NULL, NULL);
    if (!window) {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_pos_callback);
    glfwSetScrollCallback(window, scroll_callback);

    if (glfwRawMouseMotionSupported())
        glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
}

void initOpenGL() {
    glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE); // Ensure we can capture the escape key being pressed below
    glClearColor(0.1f, 0.1f, 0.3f, 0.0f);                // Dark blue background
    glEnable(GL_DEPTH_TEST);                             // Enable depth test
    glDepthFunc(GL_LESS);                                // Accept fragment if it closer to the camera than the former one
    // glEnable(GL_CULL_FACE);                              // Cull triangles which normal is not towards the camera
}

void globalInit() {

#if defined(__linux__)
    glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_X11);
#endif
    // INITIALIZE GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW!" << std::endl;
        exit(EXIT_FAILURE);
    }
    initWindow();

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
    ImGui_ImplGlfw_InitForOpenGL(window, true); // Second param install_callback=true will install GLFW callbacks and chain to existing ones.
    ImGui_ImplOpenGL3_Init();

    initOpenGL();
}
