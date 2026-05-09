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

// USUAL INCLUDES
#include <string>

struct KeyState {
    bool pressed = false;  // true only on the frame it was pressed
    bool held = false;     // true if key is held down
    bool released = false; // true only on the frame it was released
};
struct KeyCallbacks {
    std::function<void()> on_press{nullptr};
    std::function<void()> on_repeat{nullptr};
    std::function<void()> on_release{nullptr};
};

struct KeyboardHandler {
private:
    std::unordered_map<int, KeyState> m_keys;
    std::unordered_map<int, KeyCallbacks> m_callbacks;

public:
    // Register callbacks for a key (any can be null)
    void bind(int key, std::function<void()> on_press = nullptr, std::function<void()> on_repeat = nullptr, std::function<void()> on_release = nullptr) {
        m_callbacks[key] = {std::move(on_press), std::move(on_repeat), std::move(on_release)};
    }
    void unbind(int key) {
        m_callbacks.erase(key);
    }

    // Called from Window::keyCallback
    void handle(int key, int scancode, int action, int mods) {
        auto& state = m_keys[key];

        state.released = state.held && (action == GLFW_RELEASE);
        state.pressed = !state.pressed && !state.held && (action == GLFW_PRESS);
        state.held = (action == GLFW_REPEAT) || (!state.pressed && (action == GLFW_PRESS));

        auto it = m_callbacks.find(key);
        if (it != m_callbacks.end()) {
            KeyCallbacks& callbacks = it->second;
            if (action == GLFW_PRESS && callbacks.on_press)
                callbacks.on_press();
            if (action == GLFW_REPEAT && callbacks.on_repeat)
                callbacks.on_repeat();
            if (action == GLFW_RELEASE && callbacks.on_release)
                callbacks.on_release();
        }
    }

    // Poll state manually (useful for per-frame movement logic)
    const KeyState& operator[](int key) const {
        static const KeyState empty{};
        auto it = m_keys.find(key);
        return it != m_keys.end() ? it->second : empty;
    }

    bool isHeld(int key) const { return (*this)[key].held; }
};

struct Window {
private:
    std::string m_title{"Minecraft clown"};
    GLFWwindow* m_window{nullptr};
    glm::ivec2 m_pos{0, 0};
    glm::ivec2 m_size{1600, 900};
    double m_aspect_ratio{double(m_size.x) / m_size.y};
    bool m_fullscreen{false};
    glm::vec2 m_cursor_pos{0, 0};
    glm::vec2 m_cursor_vel{0, 0};
    glm::vec2 m_scroll{0, 0};

    static void _sizeCallback(GLFWwindow* w, int width, int height) { static_cast<Window*>(glfwGetWindowUserPointer(w))->sizeCallback(w, width, height); }
    static void _posCallback(GLFWwindow* w, int x, int y) { static_cast<Window*>(glfwGetWindowUserPointer(w))->posCallback(w, x, y); }
    static void _mouseButtonCallback(GLFWwindow* w, int button, int action, int mods) { static_cast<Window*>(glfwGetWindowUserPointer(w))->mouseButtonCallback(w, button, action, mods); }
    static void _cursorPosCallback(GLFWwindow* w, double xpos, double ypos) { static_cast<Window*>(glfwGetWindowUserPointer(w))->cursorPosCallback(w, xpos, ypos); }
    static void _scrollCallback(GLFWwindow* w, double xoffset, double yoffset) { static_cast<Window*>(glfwGetWindowUserPointer(w))->scrollCallback(w, xoffset, yoffset); }
    static void _keyCallback(GLFWwindow* w, int key, int scancode, int action, int mods) { static_cast<Window*>(glfwGetWindowUserPointer(w))->keyCallback(w, key, scancode, action, mods); }

    void sizeCallback(GLFWwindow* window, int width, int height) {
        // cout << "window size: " << width << ", " << height << endl;
        glViewport(0, 0, width, height);
        if (m_fullscreen)
            return;
        m_size.x = width;
        m_size.y = height;
        m_aspect_ratio = double(m_size.x) / m_size.y;
    }

    void posCallback(GLFWwindow* window, int width, int height) {
        // cout << "window pos: " << width << ", " << height << endl;
        if (m_fullscreen)
            return;
        m_pos.x = width;
        m_pos.y = height;
    }

    void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
        // cout << "mouse button:" << button << " action:" << action << " mods:" << mods << endl;
        if (button == GLFW_MOUSE_BUTTON_LEFT) {
            glfwSetInputMode(window, GLFW_CURSOR, action == GLFW_PRESS ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
        }
    }

    void cursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
        m_cursor_vel.x = xpos - m_cursor_pos.x;
        m_cursor_vel.y = ypos - m_cursor_pos.y;
        m_cursor_pos.x = xpos;
        m_cursor_pos.y = ypos;
        // cout << "m_cursor_pos: (" << m_cursor_pos.x << ", " << m_cursor_pos.y << ")\tm_cursor_vel: (" << m_cursor_vel.x << ", " << m_cursor_vel.y << ")" << endl;
    }

    void scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
        // cout << "scroll: (" << xoffset << ", " << yoffset << ")" << endl;
        m_scroll.x = xoffset;
        m_scroll.y = yoffset;
    }

    template <typename OnPressFunc, typename OnRepeatFunc, typename OnReleaseFunc>
    void _keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods, OnPressFunc&& _on_press_func, OnRepeatFunc&& _on_repeat_func, OnReleaseFunc&& _on_release_func) {
        static bool key_pressed = false;
    }

    void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
        keyboard.handle(key, scancode, action, mods);
    }

public:
    Window() = default;
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

    float m_rotation_speed{0.5f};
    float m_zoom_rate{0.05f};
    KeyboardHandler keyboard;

    inline const std::string& getTitle() const { return m_title; }
    inline GLFWwindow* getWindow() const { return m_window; }
    inline const glm::ivec2& getPos() const { return m_pos; }
    inline const glm::ivec2& getSize() const { return m_size; }
    inline double getAspectRatio() const { return m_aspect_ratio; }
    inline bool getFullscreen() const { return m_fullscreen; }
    inline const glm::vec2& getCursorPos() const { return m_cursor_pos; }
    inline const glm::vec2& getCursorVel() const { return m_cursor_vel; }
    inline const glm::vec2& getScroll() const { return m_scroll; }

    void setPos(const glm::uvec2& _pos) {
        m_pos = _pos;
        glfwSetWindowPos(m_window, m_pos.x, m_pos.y);
    }
    void setSize(const glm::uvec2& _size) {
        m_size = _size;
        glfwSetWindowSize(m_window, m_size.x, m_size.y);
    }
    void setFullscreen(bool _fullscreen) {
        m_fullscreen = _fullscreen;
        if (m_fullscreen) {
            GLFWmonitor* monitor = glfwGetPrimaryMonitor();
            const GLFWvidmode* mode = glfwGetVideoMode(monitor);
            glfwSetWindowMonitor(m_window, monitor, 0, 0, mode->width, mode->height, GLFW_DONT_CARE);
        } else {
            glfwSetWindowMonitor(m_window, nullptr, m_pos.x, m_pos.y, m_size.x, m_size.y, GLFW_DONT_CARE);
        }
    }
    inline void clearMovements() {
        m_scroll.x = m_scroll.y = 0;
        m_cursor_vel.x = m_cursor_vel.y = 0;
    }

    void init() {
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
            exit(EXIT_FAILURE);
            m_window = nullptr;
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