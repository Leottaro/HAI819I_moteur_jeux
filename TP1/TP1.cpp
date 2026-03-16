// Include standard headers
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <vector>

// Include GLEW
#include <GL/glew.h>

// Include GLFW
#include <GLFW/glfw3.h>
GLFWwindow *window;

// Include GLM
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <common/objloader.hpp>
#include <common/shader.hpp>
#include <common/vboindexer.hpp>

#include <iostream>
#include <string>
#include <vector>
#include <array>
#include "./ImageBase.h"
#include "./Mesh.hpp"
#include "./Scene.hpp"

using namespace std;

// helpers
void processInput(GLFWwindow *window);
float clipAnglePI(float _angle);
glm::vec3 EulerToEuclidian(const glm::vec2 &_angles);
glm::vec2 EuclidianToEuler(const glm::vec3 &xyz);

void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);
void cursor_pos_callback(GLFWwindow *window, double xpos, double ypos);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;
int polygon_mode = GL_FILL;
float simulation_timing = 0.f;
bool run_simulation = true;

// window
glm::vec2 cursor_pos, cursor_vel, scroll;
int window_width = 1024;
int window_height = 768;
double window_aspect_ratio = 4. / 3.;

// camera
#define NUMBER_OF_CAMERA_TYPE 3
enum CameraType {
    CameraOrbital,
    CameraFree,
    CameraAutoSpin,
};

#define M_PI_SAFE float(M_PI - 0.001)
#define M_PI_2_SAFE float(M_PI_2 - 0.001)
#define M_PI_4_SAFE float(M_PI_4 - 0.001)

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

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;
bool render_octree = false;

/*******************************************************************************/

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);
void cursor_pos_callback(GLFWwindow *window, double x, double y);

int main(void) {
    // Initialise GLFW
    if (!glfwInit()) {
        fprintf(stderr, "Failed to initialize GLFW\n");
        getchar();
        return -1;
    }

    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Open a window and create its OpenGL context
    window = glfwCreateWindow(window_width, window_height, "TP1 - GLFW", NULL, NULL);
    if (window == NULL) {
        fprintf(stderr, "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.\n");
        getchar();
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // Initialize GLEW
    glewExperimental = true; // Needed for core profile
    if (glewInit() != GLEW_OK) {
        fprintf(stderr, "Failed to initialize GLEW\n");
        getchar();
        glfwTerminate();
        return -1;
    }

    // Ensure we can capture the escape key being pressed below
    glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
    // Hide the mouse and enable unlimited mouvement
    //  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Set the mouse at the center of the screen
    glfwPollEvents();
    glfwSetCursorPos(window, 1024 / 2, 768 / 2);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetCursorPosCallback(window, cursor_pos_callback);

    // Dark blue background
    glClearColor(0.1f, 0.1f, 0.1f, 0.0f);

    // Enable depth test
    glEnable(GL_DEPTH_TEST);
    // Accept fragment if it closer to the camera than the former one
    glDepthFunc(GL_LESS);

    // Cull triangles which normal is not towards the camera
    glEnable(GL_CULL_FACE);

    GLuint VertexArrayID;
    glGenVertexArrays(1, &VertexArrayID);
    glBindVertexArray(VertexArrayID);

    // Create and compile our GLSL program from the shaders
    GLuint programID = LoadShaders("vertex_shader.glsl", "fragment_shader.glsl");

    /*****************TODO***********************/
    // Get a handle for our "Model View Projection" matrices uniforms

    // Chargement des données
    ImageBase grass("textures/grass.ppm");
    ImageBase heightmap("textures/Heightmap_Mountain.ppm");
    vector<ImageBase *> textures{&grass};

    Mesh terrain;
    glm::uvec2 terrain_resolution(5, 5);
    terrain.setSimpleTerrain(terrain_resolution, heightmap);

    uint LOD = 6;
    std::vector<Mesh> klowns(LOD);
    klowns[0].loadOFF("models/Klown.off");
    for (uint i = 1; i < LOD; i++) {
        klowns[i] = klowns[0].adaptiveSimplify(4 << i);
    }

    vector<Mesh *> meshes{&terrain};
    for (Mesh &klown : klowns) {
        meshes.push_back(&klown);
    }

    SceneNode terrain_node(0, 0);
    terrain_node.m_transfo.setScale(glm::vec3(4.f, 2.f, 4.f));
    terrain_node.m_transfo.setTranslation(glm::vec3(-2.f, -1.f, -2.f));
    SceneNode terrain_group({&terrain_node});

    SceneNode klown_node(1, -1);
    klown_node.m_transfo.setScale(0.3f);
    klown_node.m_transfo.setTranslationY(1.25f);

    SceneNode terrain_klown_node(1, -1);
    terrain_klown_node.m_transfo.setScale(0.3f);

    SceneNode root({&terrain_group, &klown_node, &terrain_klown_node});
    Scene scene(meshes, textures, &root);
    scene.initShaderData();

    // For speed computation
    double lastTime = glfwGetTime();
    int nbFrames = 0;

    do {

        // Measure speed
        // per-frame time logic
        // --------------------
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        // -----
        processInput(window);

        // Clear the screen
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Use our shader
        glUseProgram(programID);

        /****************************************/

        if (run_simulation) {
            simulation_timing += deltaTime * 2.f * M_PIf;

            klown_node.m_transfo.setTranslationX(cos(0.1f * simulation_timing));
            klown_node.m_transfo.setTranslationZ(sin(0.1f * simulation_timing));

            glm::vec3 point_on_terrain = meshes[terrain_node.m_mesh_i]->computeheight(terrain_resolution, terrain_node.m_transfo.computeTransformationMatrix(), klown_node.m_transfo.getTranslation());
            terrain_klown_node.m_transfo.setTranslation(point_on_terrain + glm::vec3(0.f, .3f, 0.f));

            float distance = glm::distance(camera_position, terrain_klown_node.m_transfo.getTranslation());
            int i = 1;
            while (i < LOD && i < distance)
                i++;
            // terrain_klown_node.m_transfo.setYaw(i * M_PIf / 8.f);
            terrain_klown_node.m_mesh_i = i;
        }

        /****************************************/

        glm::mat4 view = glm::lookAt(camera_position, camera_position + camera_front, camera_up);
        glm::mat4 projection = glm::perspective(M_PI / 4., window_aspect_ratio, 1.e-4, 1.e8);
        glUniformMatrix4fv(glGetUniformLocation(programID, "view"), 1, false, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(programID, "projection"), 1, false, glm::value_ptr(projection));

        /****************************************/

        // Draw the triangles !
        scene.render(programID, render_octree);

        // Reset some controls
        scroll = glm::vec2(0.);
        cursor_vel = glm::vec2(0.);

        // Swap buffers
        glfwSwapBuffers(window);
        glfwPollEvents();

    } // Check if the ESC key was pressed or the window was closed
    while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS &&
           glfwWindowShouldClose(window) == 0);

    // Cleanup VBO and shader
    scene.clear();

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

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
bool c_key_pressed = false;
bool w_key_pressed = false;
bool p_key_pressed = false;
bool o_key_pressed = false;
void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // Capture (or not) the cursor
    glfwSetInputMode(window, GLFW_CURSOR, (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);

    // Change camera mode
    if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS) {
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

    if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS) {
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

    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS) {
        if (!p_key_pressed) {
            p_key_pressed = true;
            run_simulation = !run_simulation;
        }
    } else {
        if (p_key_pressed) {
            p_key_pressed = false;
        }
    }

    if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS) {
        if (!o_key_pressed) {
            o_key_pressed = true;
            render_octree = !render_octree;
        }
    } else {
        if (o_key_pressed) {
            o_key_pressed = false;
        }
    }

    // Change camera rotation speed
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
        camera_rotation_speed += 1.f * deltaTime;
        cout << "rotation speed: " << camera_rotation_speed << endl;
    } else if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
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
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
            camera_position += camera_front * translation_speed;
        } else if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
            camera_position -= camera_front * translation_speed;
        } else if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
            camera_position -= camera_right * translation_speed;
        } else if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
            camera_position += camera_right * translation_speed;
        } else if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
            camera_position += real_camera_up * translation_speed;
        } else if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
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

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
    window_width = width;
    window_height = height;
    window_aspect_ratio = float(width) / float(height);
    // cout << "window: (" << window_width.x << "," << window_height.y << ")" << endl;
}

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
    scroll.x = xoffset;
    scroll.y = yoffset;
    // cout << "scroll: (" << scroll.x << "," << scroll.y << ")" << endl;
}

void cursor_pos_callback(GLFWwindow *window, double xpos, double ypos) {
    cursor_vel.x = xpos - cursor_pos.x;
    cursor_vel.y = ypos - cursor_pos.y;
    cursor_pos.x = xpos;
    cursor_pos.y = ypos;
    // cout << "cursor: (" << cursor_pos.x << "," << cursor_pos.y << ")\tvelocity: (" << cursor_vel.x << "," << cursor_vel.y << ")" << endl;
}
