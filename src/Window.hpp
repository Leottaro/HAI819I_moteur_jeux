#pragma once

// GLEW
#include <GL/glew.h>

// GLFW
#include <GLFW/glfw3.h>

// GLM
#include <glm/ext.hpp>
#include <glm/glm.hpp>
// #define GLM_ENABLE_EXPERIMENTAL
// #include <glm/gtx/string_cast.hpp>

// TODO: ca degage !
#include <iostream>

// USUAL INCLUDES
#include <mutex>
#include <string>

struct InputState {
    bool pressed{false};  // true only on the frame it was pressed
    bool held{false};     // true if key is held down
    bool released{false}; // true only on the frame it was released
};
struct InputCallbacks {
    std::function<void()> on_press{nullptr};
    std::function<void()> on_release{nullptr};
};

class InputHandler {
    std::unordered_map<int, InputState> m_inputs;
    std::unordered_map<int, InputCallbacks> m_callbacks;

public:
    InputHandler() {}

    // Register callbacks for a key (any can be null)
    inline void bind(int key, std::function<void()> on_press = nullptr, std::function<void()> on_release = nullptr) {
        m_callbacks[key] = {std::move(on_press), std::move(on_release)};
    }
    inline void unbind(int key) {
        m_callbacks.erase(key);
    }

    // Called from Window::keyCallback
    void handle(int input, int action, int mods) {
        auto& state = m_inputs[input];

        bool new_pressed = !state.pressed && !state.held && (action == GLFW_PRESS);
        bool new_held = (action == GLFW_REPEAT) || ((state.pressed || state.held) && (action == GLFW_PRESS));
        bool new_released = (state.held || state.pressed) && (action == GLFW_RELEASE);
        state.pressed = new_pressed;
        state.held = new_held;
        state.released = new_released;

        auto it = m_callbacks.find(input);
        if (it != m_callbacks.end()) {
            InputCallbacks& callbacks = it->second;
            if (action == GLFW_PRESS && callbacks.on_press)
                callbacks.on_press();
            if (action == GLFW_RELEASE && callbacks.on_release)
                callbacks.on_release();
        }
    }

    // Poll state manually (useful for per-frame movement logic)
    inline const InputState& getState(int key) const {
        static const InputState empty{};
        auto it = m_inputs.find(key);
        return it != m_inputs.end() ? it->second : empty;
    }

    inline bool isHeld(int key) const { return getState(key).pressed || getState(key).held; }
};

struct Window {
    inline static std::mutex glfw_mutex;

    static Window* getWindow(GLFWwindow* w) {
        std::lock_guard<std::mutex> lock(glfw_mutex);
        return static_cast<Window*>(glfwGetWindowUserPointer(w));
    }

    static void _sizeCallback(GLFWwindow* w, int width, int height) { getWindow(w)->sizeCallback(w, width, height); }
    static void _posCallback(GLFWwindow* w, int x, int y) { getWindow(w)->posCallback(w, x, y); }
    static void _mouseButtonCallback(GLFWwindow* w, int button, int action, int mods) { getWindow(w)->mouseButtonCallback(w, button, action, mods); }
    static void _cursorPosCallback(GLFWwindow* w, double xpos, double ypos) { getWindow(w)->cursorPosCallback(w, xpos, ypos); }
    static void _scrollCallback(GLFWwindow* w, double xoffset, double yoffset) { getWindow(w)->scrollCallback(w, xoffset, yoffset); }
    static void _keyCallback(GLFWwindow* w, int key, int scancode, int action, int mods) { getWindow(w)->keyCallback(w, key, scancode, action, mods); }

private:
    bool m_mouse_captured{false};
    std::string m_title{"Minecraft clown"};
    GLFWwindow* m_window{nullptr};
    glm::ivec2 m_pos{0, 0};
    glm::ivec2 m_size{1600, 900};
    glm::ivec2 m_effective_size{1600, 900};
    double m_aspect_ratio{double(m_size.x) / m_size.y};
    bool m_fullscreen{false};
    glm::vec2 m_cursor_pos{0, 0};
    glm::vec2 m_cursor_vel{0, 0};
    glm::vec2 m_scroll{0, 0};

    inline void sizeCallback(GLFWwindow* window, int width, int height) {
        // cout << "window size: " << width << ", " << height << endl;
        glViewport(0, 0, width, height);
        if (m_fullscreen)
            return;
        m_size.x = width;
        m_size.y = height;
        m_aspect_ratio = double(m_size.x) / m_size.y;
    }

    inline void posCallback(GLFWwindow* window, int width, int height) {
        // cout << "window pos: " << width << ", " << height << endl;
        if (m_fullscreen)
            return;
        m_pos.x = width;
        m_pos.y = height;
    }

    inline void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
        mouse.handle(button, action, mods);
    }

    inline void cursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
        m_cursor_vel.x = xpos - m_cursor_pos.x;
        m_cursor_vel.y = ypos - m_cursor_pos.y;
        m_cursor_pos.x = xpos;
        m_cursor_pos.y = ypos;
        // cout << "m_cursor_pos: (" << m_cursor_pos.x << ", " << m_cursor_pos.y << ")\tm_cursor_vel: (" << m_cursor_vel.x << ", " << m_cursor_vel.y << ")" << endl;
    }

    inline void scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
        // cout << "scroll: (" << xoffset << ", " << yoffset << ")" << endl;
        m_scroll.x = xoffset;
        m_scroll.y = yoffset;
    }

    inline void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
        keyboard.handle(key, action, mods);
    }

public:
    Window() {}
    Window(Window&&) = delete;
    Window(const Window&) = delete;
    Window& operator=(Window&&) = delete;
    Window& operator=(const Window&) = delete;
    ~Window() {
        if (m_window) {
            glfwTerminate();
            m_window = nullptr;
        }
    }

    InputHandler keyboard;
    InputHandler mouse;

    inline const std::string& getTitle() const { return m_title; }
    inline GLFWwindow* getWindow() const { return m_window; }
    inline const glm::ivec2& getPos() const { return m_pos; }
    inline const glm::ivec2& getSize() const { return m_size; }
    inline const glm::ivec2& getEffectiveSize() const { return m_effective_size; }
    inline double getAspectRatio() const { return m_aspect_ratio; }
    inline bool getFullscreen() const { return m_fullscreen; }
    inline const glm::vec2& getCursorPos() const { return m_cursor_pos; }
    inline const glm::vec2& getCursorVel() const { return m_cursor_vel; }
    inline const glm::vec2& getScroll() const { return m_scroll; }

    inline const bool getMouseCapture() const { return m_mouse_captured; }
    inline void toggleMouseCapture() {
        m_mouse_captured = !m_mouse_captured;
        glfwSetInputMode(m_window, GLFW_CURSOR, m_mouse_captured ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
        if (m_mouse_captured) {
            mouse.bind(GLFW_MOUSE_BUTTON_LEFT);
            mouse.bind(GLFW_MOUSE_BUTTON_RIGHT);
            mouse.bind(GLFW_MOUSE_BUTTON_MIDDLE);
            if (glfwRawMouseMotionSupported())
                glfwSetInputMode(m_window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
        } else {
            mouse.unbind(GLFW_MOUSE_BUTTON_LEFT);
            mouse.unbind(GLFW_MOUSE_BUTTON_RIGHT);
            mouse.unbind(GLFW_MOUSE_BUTTON_MIDDLE);
            if (glfwRawMouseMotionSupported())
                glfwSetInputMode(m_window, GLFW_RAW_MOUSE_MOTION, GLFW_FALSE);
        }
    }

    void setPos(const glm::uvec2& _pos) {
        std::lock_guard<std::mutex> lock(glfw_mutex);
        m_pos = _pos;
        glfwSetWindowPos(m_window, m_pos.x, m_pos.y);
    }
    void setSize(const glm::uvec2& _size) {
        std::lock_guard<std::mutex> lock(glfw_mutex);
        m_size = _size;
        glfwSetWindowSize(m_window, m_size.x, m_size.y);
    }
    void setFullscreen(bool _fullscreen) {
        std::lock_guard<std::mutex> lock(glfw_mutex);
        m_fullscreen = _fullscreen;
        if (m_fullscreen) {
            GLFWmonitor* monitor = glfwGetPrimaryMonitor();
            const GLFWvidmode* mode = glfwGetVideoMode(monitor);
            glfwSetWindowMonitor(m_window, monitor, 0, 0, mode->width, mode->height, GLFW_DONT_CARE);
            m_effective_size.x = mode->width;
            m_effective_size.y = mode->height;
        } else {
            glfwSetWindowMonitor(m_window, nullptr, m_pos.x, m_pos.y, m_size.x, m_size.y, GLFW_DONT_CARE);
            m_effective_size = m_size;
        }
    }
    inline void clearMovements() {
        m_scroll.x = m_scroll.y = 0;
        m_cursor_vel.x = m_cursor_vel.y = 0;
    }

    inline void init() {
        std::lock_guard<std::mutex> lock(glfw_mutex);
        glfwWindowHint(GLFW_SAMPLES, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_SCALE_FRAMEBUFFER, GL_FALSE); // https://discourse.glfw.org/t/resizing-window-results-in-wrong-aspect-ratio/1268s
        glfwWindowHint(GLFW_DEPTH_BITS, 24);

        m_window = glfwCreateWindow(m_size.x, m_size.y, m_title.c_str(), NULL, NULL);
        if (!m_window) {
            glfwTerminate();
            m_window = nullptr;
            exit(EXIT_FAILURE);
        }
        glfwMakeContextCurrent(m_window);
        glfwSetWindowUserPointer(m_window, this);
        glfwSetWindowSizeCallback(m_window, _sizeCallback);
        glfwSetWindowPosCallback(m_window, _posCallback);
        glfwSetKeyCallback(m_window, _keyCallback);
        glfwSetMouseButtonCallback(m_window, _mouseButtonCallback);
        glfwSetCursorPosCallback(m_window, _cursorPosCallback);
        glfwSetScrollCallback(m_window, _scrollCallback);
        glfwSetInputMode(m_window, GLFW_STICKY_KEYS, GL_TRUE);
        glfwSwapInterval(1); // VSync
    }
};