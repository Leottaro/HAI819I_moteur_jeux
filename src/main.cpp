// GLEW
#include <GL/glew.h>

// GLM
#include <glm/ext.hpp>
#include <glm/glm.hpp>

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
#include <vector>
#include <fstream>
#include "ShaderProgram.hpp"
#include "ImageBase.h"
#include "Mesh.hpp"
#include "Scene.hpp"
#include "Camera.hpp"
#include "RigidBody.hpp"

using namespace std;

// window state
GLuint window_width = 800, window_height = 600;
double window_aspect_ratio = double(window_width) / window_height;
GLenum polygon_mode = GL_FILL;
glm::vec2 cursor_pos{0, 0};
glm::vec2 cursor_vel{0, 0};
glm::vec2 scroll{0, 0};

bool is_dragging = false;
glm::vec2 original_drag_pos{0, 0};
glm::vec2 original_center_pos{0, 0};

bool run_simulation = false;
float deltaTime = 0.f;
float lastFrame = 0.f;

Camera camera;

RigidBody cube_body;
ofstream logfile;

void globalInit();

int main(void) {
    globalInit();

    // Create and compile our GLSL program from the shaders
    ShaderProgram shader("ressources/shaders/vertex_shader.glsl", "ressources/shaders/fragment_shader.glsl");

    // Chargement des données
    ImageBase grass("ressources/textures/grass.ppm");
    vector<ImageBase *> textures{&grass};

    Mesh terrain;
    glm::uvec2 terrain_resolution(512, 512);
    // ImageBase heightmap("ressources/textures/Heightmap_Mountain.ppm");
    // terrain.setSimpleTerrain(terrain_resolution, heightmap);
    terrain.setSimpleGrid(terrain_resolution);

    Mesh cube_mesh;
    cube_mesh.setCube(2);

    vector<Mesh *> meshes{&terrain, &cube_mesh};

    SceneNode terrain_node(0, 0);
    terrain_node.m_transfo.setScale(glm::vec3(1000.f, 1.f, 1000.f));
    terrain_node.m_transfo.setTranslation(glm::vec3(-5.f, 0.f, -5.f));
    terrain_node.m_transfo.setEulerAngles(glm::vec3(M_PI_4f, 0.f, 0.f));
    float static_friction = 0.5f;
    SceneNode terrain_group({&terrain_node});

    SceneNode cube_node(1, -1);
    cube_node.m_transfo.setTranslation(cube_body.m_pos);
    cube_node.m_transfo.setEulerAngles(glm::vec3(atanf(sqrtf(2.f)), M_PI / 4.f, 0.f));

    SceneNode root({&terrain_group, &cube_node});
    Scene scene(meshes, textures, &root);
    scene.initShaderData();

    camera.m_center = &cube_node.m_transfo.getTranslation();

    logfile = ofstream("log.log");

    // glfwSwapInterval(1); // VSync - avoid having 3000 fps
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
            cube_body.update(deltaTime, {glm::vec3(0.f, -9.81f, 0.f)});

            auto [cube_on_terrain, triangle_normal] = meshes[terrain_node.m_mesh_i]->computeheight(terrain_resolution, terrain_node.m_transfo.computeTransformationMatrix(), cube_body.m_pos);
            float dist_to_terrain = glm::dot(cube_body.m_pos - cube_on_terrain, triangle_normal);

            if (dist_to_terrain < 0.f) {
                cube_body.m_pos = cube_on_terrain;
                cube_body.bounce(static_friction, applyTransformation(triangle_normal, 0.f, terrain_node.m_transfo.computeTransformationMatrix()));
            }
        }
        cube_node.m_transfo.setTranslation(cube_body.m_pos + glm::vec3(0.f, sqrt(3.f) / 2.f, 0.f));

        /**********==========RENDERING==========**********/
        shader.use();

        shader.set("view", camera.getViewMatrix());
        shader.set("projection", camera.getProjectionMatrix());

        scene.render(shader);

        // ImGui Render
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Reset some controls
        scroll = glm::vec2(0.);
        cursor_vel = glm::vec2(0.);
        logfile << cube_body.m_pos.x << "," << cube_body.m_pos.y << "," << cube_body.m_pos.z << "," << cube_body.m_vel.x << "," << cube_body.m_vel.y << "," << cube_body.m_vel.z << std::endl;
    } while (glfwWindowShouldClose(window) == GLFW_FALSE);

    // Cleanup VBO and shader
    scene.clear();
    shader.~ShaderProgram();
    logfile.close();

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

bool c_key_pressed = false;
bool w_key_pressed = false;
bool p_key_pressed = false;
bool space_key_pressed = false;
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

    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
        if (!space_key_pressed) {
            space_key_pressed = true;

            cube_body.m_pos = camera.m_position;

            glm::mat4 rot = glm::rotate(glm::mat4(1.), M_PIf / 4.f, glm::normalize(glm::cross(camera.getFront(), camera.getUp())));
            cube_body.m_vel = 10.f * applyTransformation(camera.getFront(), 0.f, rot);
        }
    } else {
        if (space_key_pressed) {
            space_key_pressed = false;
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
    glEnable(GL_CULL_FACE);                              // Cull triangles which normal is not towards the camera
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
