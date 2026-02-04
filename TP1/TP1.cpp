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
#include <iostream>

#include <common/objloader.hpp>
#include <common/shader.hpp>
#include <common/vboindexer.hpp>

#include <string>
#include <vector>
#include "./ImageBase.h"

using namespace std;

// helpers
void processInput(GLFWwindow *window);
float clipAnglePI(float _angle);
glm::vec3 EulerToEuclidian(const glm::vec2 &_angles);
glm::vec2 EuclidianToEuler(const glm::vec3 &xyz);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// window
glm::vec2 cursor_pos, cursor_vel, scroll;
int window_width = 1024;
int window_height = 768;

// camera
#define M_PI_SAFE float(M_PI - 0.001)
#define M_PI_2_SAFE float(M_PI_2 - 0.001)
#define M_PI_4_SAFE float(M_PI_4 - 0.001)

glm::vec3 camera_position = glm::vec3(1.0f, 1.0f, 1.0f);
glm::vec2 camera_angles = glm::vec2(-M_PI_4 * 0.5, 0.); // (pitch, yaw)
glm::vec3 camera_up = glm::vec3(0.0f, 1.0f, 0.0f);
glm::vec3 camera_front = EulerToEuclidian(camera_angles);

bool camera_is_orbital = true;
float camera_rotation_speed = 1.0f;
float camera_translation_speed = 2.5f;

// camera orbital
glm::vec3 camera_center = glm::vec3(0.f, 0.5f, 0.f);
float camera_distance_to_center = 3.;

// timing
float deltaTime = 0.0f; // time between current frame and last frame
float lastFrame = 0.0f;

/*******************************************************************************/

// Chargement des textures
vector<ImageBase> textures = vector<ImageBase>(3);
vector<ImageBase> heightmaps = vector<ImageBase>(3);

unsigned int plane_size = 10;
unsigned int current_heightmap = 0;

vector<unsigned int> indices; // Triangles concaténés dans une liste
vector<vector<unsigned int>> triangles;
vector<glm::vec3> indexed_vertices;
vector<glm::vec2> indexed_uvs;

GLuint vertexbuffer;
GLuint uvbuffer;
GLuint elementbuffer;

void createPlaneGeometry() {
    cout << "terrain size: " << plane_size << endl;
    indexed_vertices.resize(0);
    indexed_uvs.resize(0);
    indices.resize(0);
    triangles.resize(0);

    for (unsigned int nY = 0; nY < plane_size; nY++) {
        float v = float(nY) / (plane_size - 1);
        for (unsigned int nX = 0; nX < plane_size; nX++) {
            float u = float(nX) / (plane_size - 1);
            glm::vec2 uv = glm::vec2(float(nX) / float(plane_size), float(nY) / float(plane_size));
            indexed_uvs.push_back(uv);

            float altitude = float(heightmaps[current_heightmap].getPixel(u, v)[0]) / 255.;
            glm::vec3 vertex = glm::vec3(1. - 2. * uv.x, altitude, 1. - 2. * uv.y);
            indexed_vertices.push_back(vertex);
        }
    }

    for (unsigned int nY = 0; nY < plane_size - 1; nY++) {
        for (unsigned int nX = 0; nX < plane_size - 1; nX++) {
            unsigned int i1 = nY * plane_size + nX;
            unsigned int i2 = (nY + 1) * plane_size + nX;
            unsigned int i3 = nY * plane_size + (nX + 1);
            unsigned int i4 = (nY + 1) * plane_size + (nX + 1);
            vector<unsigned int> triangle1 = {i1, i2, i3};
            vector<unsigned int> triangle2 = {i2, i4, i3};

            indices.push_back(i1);
            indices.push_back(i2);
            indices.push_back(i3);
            triangles.push_back(triangle1);

            indices.push_back(i2);
            indices.push_back(i4);
            indices.push_back(i3);
            triangles.push_back(triangle2);
        }
    }

    glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
    glBufferData(GL_ARRAY_BUFFER, indexed_vertices.size() * sizeof(glm::vec3), &indexed_vertices[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
    glBufferData(GL_ARRAY_BUFFER, indexed_uvs.size() * sizeof(glm::vec2), &indexed_uvs[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementbuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);
}

void drawPlane(vector<glm::vec3> &indexed_vertices, vector<unsigned int> &indices) {
    glDrawElements(
        GL_TRIANGLES,    // mode
        indices.size(),  // count
        GL_UNSIGNED_INT, // type
        (void *)0        // element array buffer offset
    );
}

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
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetCursorPosCallback(window, cursor_pos_callback);

    // Dark blue background
    glClearColor(0.8f, 0.8f, 0.8f, 0.0f);

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

    // Chargement des textures
    textures[0].load("textures/grass.ppm");
    textures[1].load("textures/rock.ppm");
    textures[2].load("textures/snowrocks.ppm");
    heightmaps[0].load("textures/heightmap-mountain.pgm");
    heightmaps[1].load("textures/heightmap-rocky.pgm");
    heightmaps[2].load("textures/heightmap-test.pgm");

    // Load it into a VBO
    glGenBuffers(1, &vertexbuffer);
    glGenBuffers(1, &uvbuffer);
    glGenBuffers(1, &elementbuffer);

    // Chargement du fichier de maillage
    createPlaneGeometry();

    // Get a handle for our "LightPosition" uniform
    glUseProgram(programID);
    GLuint LightID = glGetUniformLocation(programID, "LightPosition_worldspace");
    glUniform1i(glGetUniformLocation(programID, "grass"), 0);
    glUniform1i(glGetUniformLocation(programID, "rock"), 1);
    glUniform1i(glGetUniformLocation(programID, "snowrocks"), 2);
    glUniform1i(glGetUniformLocation(programID, "heightmap_mountain"), 3);
    glUniform1i(glGetUniformLocation(programID, "heightmap_rocky"), 4);
    glUniform1i(glGetUniformLocation(programID, "heightmap_test"), 5);

    // TEXTURES
    for (int i = 0; i < textures.size(); i++) {
        GLuint texture;
        glGenTextures(1, &texture);
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

        if (textures[i].getColor()) {
            vector<float> image_data(textures[i].getWidth() * textures[i].getHeight() * 4);
            for (size_t j = 0; j < textures[i].getWidth() * textures[i].getHeight(); j++) {
                const unsigned char *pixel = textures[i].getPixel(j);
                image_data[j * 4] = float(pixel[0]) / 255.;
                image_data[j * 4 + 1] = float(pixel[1]) / 255.;
                image_data[j * 4 + 2] = float(pixel[2]) / 255.;
                image_data[j * 4 + 3] = 1.;
            }
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, textures[i].getWidth(), textures[i].getHeight(), 0, GL_RGBA, GL_FLOAT, image_data.data());
            glBindImageTexture(i, texture, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
        } else {
            vector<float> image_data(textures[i].getWidth() * textures[i].getHeight());
            for (size_t j = 0; j < textures[i].getWidth() * textures[i].getHeight(); j++) {
                const unsigned char *pixel = textures[i].getPixel(j);
                image_data[j] = float(pixel[0]) / 255.;
            }
            glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, textures[i].getWidth(), textures[i].getHeight(), 0, GL_RED, GL_FLOAT, image_data.data());
            glBindImageTexture(0, texture, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R32F);
        }
    }

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

        // Model matrix : an identity matrix (model will be at the origin) then change
        glm::mat4 model = glm::mat4();
        glm::mat4 view = glm::lookAt(camera_position, camera_position + camera_front, camera_up);
        // Projection matrix : 45 Field of View, 4:3 ratio, display range : 0.1 unit <-> 100 units
        glm::mat4 projection = glm::perspective(M_PI / 4., 4. / 3., 0.1, 100.);

        // Send our transformation to the currently bound shader,
        // in the "Model View Projection" to the shader uniforms
        glm::mat4 MVP = projection * view * model;
        glUniformMatrix4fv(glGetUniformLocation(programID, "MVP"), 1, false, glm::value_ptr(MVP));

        glUniform1ui(glGetUniformLocation(programID, "sampler"), 0);

        /****************************************/

        // 1rst attribute buffer : vertices
        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
        glVertexAttribPointer(
            0,        // attribute
            3,        // size
            GL_FLOAT, // type
            GL_FALSE, // normalized?
            0,        // stride
            (void *)0 // array buffer offset
        );

        glEnableVertexAttribArray(1);
        glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
        glVertexAttribPointer(
            1,        // attribute
            2,        // size
            GL_FLOAT, // type
            GL_FALSE, // normalized?
            0,        // stride
            (void *)0 // array buffer offset
        );

        // Index buffer
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementbuffer);

        // Draw the triangles !
        drawPlane(indexed_vertices, indices);

        glDisableVertexAttribArray(0);

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
    glDeleteBuffers(1, &vertexbuffer);
    glDeleteBuffers(1, &uvbuffer);
    glDeleteBuffers(1, &elementbuffer);
    glDeleteProgram(programID);
    glDeleteVertexArrays(1, &VertexArrayID);

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
void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // Change camera mode
    if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS) {
        if (!c_key_pressed) {
            camera_is_orbital = !camera_is_orbital;
            cout << "orbital: " << (camera_is_orbital ? "true" : "false") << endl;
            c_key_pressed = true;
            camera_distance_to_center = glm::distance(camera_position, camera_center);
            camera_front = glm::normalize(camera_center - camera_position);
            camera_angles = EuclidianToEuler(camera_front);
        }
    } else {
        if (c_key_pressed) {
            c_key_pressed = false;
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
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        camera_angles = glm::vec2(
            glm::clamp(camera_angles.x - rotation_speed * cursor_vel.y, -M_PI_2_SAFE, M_PI_2_SAFE),
            clipAnglePI(camera_angles.y - rotation_speed * cursor_vel.x));

        camera_front = EulerToEuclidian(camera_angles);
    }

    float translation_speed = camera_translation_speed * deltaTime;
    if (camera_is_orbital) {
        camera_distance_to_center = glm::max(camera_distance_to_center - translation_speed * scroll.y, 1.f);
        camera_position = camera_center - camera_distance_to_center * camera_front;
    } else {
        glm::vec3 camera_right = glm::cross(camera_front, camera_up);
        // glm::vec3 real_camera_up = glm::cross(camera_right, camera_front);
        if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
            camera_position += camera_front * translation_speed;
        } else if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
            camera_position -= camera_front * translation_speed;
        } else if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
            camera_position -= camera_right * translation_speed;
        } else if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
            camera_position += camera_right * translation_speed;
        } else if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
            camera_position += camera_up * translation_speed;
        } else if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
            camera_position -= camera_up * translation_speed;
        }
    }

    // Resolution change
    if (glfwGetKey(window, GLFW_KEY_KP_ADD) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_EQUAL) == GLFW_PRESS) {
        plane_size++;
        createPlaneGeometry();
    } else if (glfwGetKey(window, GLFW_KEY_KP_SUBTRACT) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_6) == GLFW_PRESS) {
        plane_size = std::max(2, int(plane_size - 1));
        createPlaneGeometry();
    }

    // TODO add translations
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
    window_width = width;
    window_height = height;
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