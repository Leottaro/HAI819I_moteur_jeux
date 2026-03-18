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
#include "ShaderProgram.hpp"
#include "ImageBase.h"
#include "Mesh.hpp"
#include "Scene.hpp"

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

// camera
float clipAnglePI(float _angle);
glm::vec3 EulerToEuclidian(const glm::vec2 &_angles);
glm::vec2 EuclidianToEuler(const glm::vec3 &xyz);
#define NUMBER_OF_CAMERA_TYPE 3
enum CameraType {
    CameraOrbital,
    CameraFree,
    CameraAutoSpin,
};

#define M_PI_SAFE float(M_PI - 0.001)
#define M_PI_2_SAFE float(M_PI_2 - 0.001)
#define M_PI_4_SAFE float(M_PI_4 - 0.001)

double camera_fov = M_PI_2;
glm::vec3 camera_position = glm::vec3(1.0f, 1.0f, 1.0f);
glm::vec2 camera_angles = glm::vec2(-M_PI_4 * 0.5, 0.); // (pitch, yaw)
glm::vec3 camera_up = glm::vec3(0.0f, 1.0f, 0.0f);
glm::vec3 camera_front = EulerToEuclidian(camera_angles);

CameraType camera_type = CameraOrbital;
float camera_rotation_speed = 1.0f;
float camera_translation_speed = 2.5f;

// camera orbital
glm::vec3 camera_center = glm::vec3(0.f, 0.f, 0.f);
float camera_distance_to_center = 5.f;

float cube_bounciness = 0.9f;
float cube_weight = 1.f;
glm::vec3 cube_pos = glm::vec3(0.f, 0.f, 0.f);
glm::vec3 cube_vel = glm::vec3(0.f, 0.f, 0.f);
glm::vec3 cube_accel = glm::vec3(0.f, 0.f, 0.f);

void globalInit();

int main(void) {
    globalInit();

    // Create and compile our GLSL program from the shaders
    ShaderProgram shader("ressources/shaders/vertex_shader.glsl", "ressources/shaders/fragment_shader.glsl");

    // Chargement des données
    ImageBase grass("ressources/textures/grass.ppm");
    vector<ImageBase *> textures{&grass};

    Mesh terrain;
    glm::uvec2 terrain_resolution(16, 16);
    ImageBase heightmap("ressources/textures/Heightmap_Mountain.ppm");
    terrain.setSimpleTerrain(terrain_resolution, heightmap);
    // terrain.setSimpleGrid(terrain_resolution);

    Mesh cube;
    cube.setCube(2);

    vector<Mesh *> meshes{&terrain, &cube};

    SceneNode terrain_node(0, 0);
    terrain_node.m_transfo.setScale(glm::vec3(100.f, 50.f, 100.f));
    terrain_node.m_transfo.setTranslation(glm::vec3(-50.f, -50.f, -50.f));
    SceneNode terrain_group({&terrain_node});

    SceneNode cube_node(1, -1);
    cube_node.m_transfo.setTranslation(cube_pos);
    cube_node.m_transfo.setEulerAngles(glm::vec3(atanf(sqrtf(2.f)), M_PI / 4.f, 0.f));

    SceneNode root({&terrain_group, &cube_node});
    Scene scene(meshes, textures, &root);
    scene.initShaderData();

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
        float rotation_speed = camera_rotation_speed * deltaTime;
        float translation_speed = camera_translation_speed * deltaTime;
        glm::vec3 camera_right = glm::cross(camera_front, camera_up);
        // glm::vec3 real_camera_up = glm::cross(camera_right, camera_front);
        switch (camera_type) {
        case CameraOrbital:
            if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
                camera_angles = glm::vec2(
                    glm::clamp(camera_angles.x - rotation_speed * cursor_vel.y, -M_PI_2_SAFE, M_PI_2_SAFE),
                    clipAnglePI(camera_angles.y - rotation_speed * cursor_vel.x));

                camera_front = EulerToEuclidian(camera_angles);
            }
            camera_distance_to_center = glm::max(camera_distance_to_center - translation_speed * scroll.y, .1f);
            camera_position = camera_center - camera_distance_to_center * camera_front;
            break;
        case CameraFree:
            if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
                camera_angles = glm::vec2(
                    glm::clamp(camera_angles.x - rotation_speed * cursor_vel.y, -M_PI_2_SAFE, M_PI_2_SAFE),
                    clipAnglePI(camera_angles.y - rotation_speed * cursor_vel.x));

                camera_front = EulerToEuclidian(camera_angles);
            }
            if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
                camera_position += camera_front * translation_speed;
            } else if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
                camera_position -= camera_front * translation_speed;
            } else if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
                camera_position -= camera_right * translation_speed;
            } else if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
                camera_position += camera_right * translation_speed;
            }
            break;
        case CameraAutoSpin:
            camera_distance_to_center = glm::max(camera_distance_to_center - translation_speed * scroll.y, 1.f);
            camera_angles.y = clipAnglePI(camera_angles.y + rotation_speed);
            camera_front = EulerToEuclidian(camera_angles);
            camera_position = camera_center - camera_distance_to_center * camera_front;
            break;
        }

        /**********==========OBJECTS UPDATE==========**********/
        if (run_simulation) {
            cube_accel = glm::vec3(0.f);
            cube_accel += glm::vec3(0.f, -9.81f, 0.f) * 0.5f;
            cube_accel /= cube_weight;

            cube_vel += deltaTime * cube_accel;
            cube_pos += cube_vel;

            auto [cube_on_terrain, triangle_normal] = meshes[terrain_node.m_mesh_i]->computeheight(terrain_resolution, terrain_node.m_transfo.computeTransformationMatrix(), cube_pos);
            float dist_to_terrain = glm::dot(cube_pos - cube_on_terrain, triangle_normal);

            if (dist_to_terrain < 0.f) {
                cube_pos = cube_on_terrain;

                // Reflection
                float vel_dot_n = glm::dot(cube_vel, triangle_normal);
                glm::vec3 v_normal = vel_dot_n * triangle_normal;
                cube_vel = cube_vel - (1.f + cube_bounciness) * v_normal;
            }
        }
        cube_node.m_transfo.setTranslation(cube_pos + glm::vec3(0.f, sqrt(3.f) / 2.f, 0.f));

        /**********==========RENDERING==========**********/
        shader.use();

        glm::mat4 view = glm::lookAt(camera_position, camera_position + camera_front, camera_up);
        glm::mat4 projection = glm::perspective(camera_fov, window_aspect_ratio, 1.e-4, 1.e8);
        shader.set("view", view);
        shader.set("projection", projection);

        scene.render(shader);

        // ImGui Render
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Reset some controls
        scroll = glm::vec2(0.);
        cursor_vel = glm::vec2(0.);
    } while (glfwWindowShouldClose(window) == GLFW_FALSE);

    // Cleanup VBO and shader
    scene.clear();
    shader.~ShaderProgram();

    // Close OpenGL window and terminate GLFW
    glfwTerminate();

    return 0;
}

// helpers
float clipAnglePI(float _angle) {
    while (_angle < -M_PI)
        _angle += 2. * M_PI;
    while (_angle > M_PI)
        _angle -= 2. * M_PI;
    return _angle;
}

glm::vec3 EulerToEuclidian(const glm::vec2 &_angles) {
    float sinPhi = cosf(_angles.x);
    float x = sinPhi * sinf(_angles.y);
    float y = sinf(_angles.x);
    float z = sinPhi * cosf(_angles.y);

    return glm::vec3(x, y, z);
}

glm::vec2 EuclidianToEuler(const glm::vec3 &xyz) {
    float angles_x = asin(xyz[1] / glm::length(xyz)); // polar angle from +y axis, 0..π

    float angles_y = atan2(xyz[0], xyz[2]); // azimuth around y-axis, 0..2π
    if (angles_y < 0.0f)
        angles_y += 2.0f * M_PI;

    return glm::vec2(angles_x, angles_y);
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

    // Change camera mode
    if (key == GLFW_KEY_C && action == GLFW_PRESS) {
        if (!c_key_pressed) {
            camera_type = CameraType((int(camera_type) + 1) % NUMBER_OF_CAMERA_TYPE);
            c_key_pressed = true;
            string camera_type_str;
            switch (camera_type) {
            case CameraOrbital:
                camera_distance_to_center = glm::distance(camera_position, camera_center);
                camera_front = glm::normalize(camera_center - camera_position);
                camera_angles = EuclidianToEuler(camera_front);
                camera_type_str = "Orbital";
                break;
            case CameraFree:
                camera_type_str = "Free";
                break;
            case CameraAutoSpin:
                camera_angles = glm::vec2(-M_PI_4 * 0.5, 0.);
                camera_type_str = "AutoSpin";
                break;
            }
            glfwSetWindowTitle(window, ("TP1 - Camera mode: " + camera_type_str).c_str());
        }
    } else {
        if (c_key_pressed) {
            c_key_pressed = false;
        }
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

            cube_pos = camera_position;

            glm::mat4 rot = glm::rotate(glm::mat4(1.), M_PIf / 4.f, glm::normalize(glm::cross(camera_front, camera_up)));
            glm::vec4 new_vel = rot * glm::vec4(camera_front.x, camera_front.y, camera_front.z, 0.f);
            cube_vel = glm::normalize(glm::vec3(new_vel.x, new_vel.y, new_vel.z));
        }
    } else {
        if (space_key_pressed) {
            space_key_pressed = false;
        }
    }

    // Change camera rotation speed
    if (key == GLFW_KEY_UP && action == GLFW_PRESS) {
        camera_rotation_speed += 1.f * deltaTime;
        cout << "rotation speed: " << camera_rotation_speed << endl;
    } else if (key == GLFW_KEY_DOWN && action == GLFW_PRESS) {
        camera_rotation_speed = max(camera_rotation_speed - 1.f * deltaTime, 0.f);
        cout << "rotation speed: " << camera_rotation_speed << endl;
    }

    // Camera update
    float rotation_speed = camera_rotation_speed * deltaTime;
    float translation_speed = camera_translation_speed * deltaTime;
    glm::vec3 camera_right = glm::cross(camera_front, camera_up);
    glm::vec3 real_camera_up = glm::cross(camera_right, camera_front);
    switch (camera_type) {
    case CameraOrbital:
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
            camera_angles = glm::vec2(
                glm::clamp(camera_angles.x - rotation_speed * cursor_vel.y, -M_PI_2_SAFE, M_PI_2_SAFE),
                clipAnglePI(camera_angles.y - rotation_speed * cursor_vel.x));

            camera_front = EulerToEuclidian(camera_angles);
        }
        camera_distance_to_center = glm::max(camera_distance_to_center - translation_speed * scroll.y, .1f);
        camera_position = camera_center - camera_distance_to_center * camera_front;
        break;
    case CameraFree:
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
            camera_angles = glm::vec2(
                glm::clamp(camera_angles.x - rotation_speed * cursor_vel.y, -M_PI_2_SAFE, M_PI_2_SAFE),
                clipAnglePI(camera_angles.y - rotation_speed * cursor_vel.x));

            camera_front = EulerToEuclidian(camera_angles);
        }
        if (key == GLFW_KEY_W && action == GLFW_PRESS) {
            camera_position += camera_front * translation_speed;
        } else if (key == GLFW_KEY_S && action == GLFW_PRESS) {
            camera_position -= camera_front * translation_speed;
        } else if (key == GLFW_KEY_A && action == GLFW_PRESS) {
            camera_position -= camera_right * translation_speed;
        } else if (key == GLFW_KEY_D && action == GLFW_PRESS) {
            camera_position += camera_right * translation_speed;
        } else if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
            camera_position += real_camera_up * translation_speed;
        } else if (key == GLFW_KEY_LEFT_CONTROL && action == GLFW_PRESS) {
            camera_position -= real_camera_up * translation_speed;
        }
        break;
    case CameraAutoSpin:
        camera_distance_to_center = glm::max(camera_distance_to_center - translation_speed * scroll.y, 1.f);
        camera_angles.y = clipAnglePI(camera_angles.y + rotation_speed);
        camera_front = EulerToEuclidian(camera_angles);
        camera_position = camera_center - camera_distance_to_center * camera_front;
        break;
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