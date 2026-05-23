#pragma once

// GLEW
#include <GL/glew.h>

#include <vector>
#include <mutex>

class GLGlobalContext {
private:
    std::vector<GLuint> arrays_to_delete;
    std::vector<GLuint> buffers_to_delete;
    std::mutex gc_mutex;

public:
    inline void addArrayToDelete(GLuint array) {
        std::lock_guard<std::mutex> lock(gc_mutex);
        arrays_to_delete.push_back(array);
    }
    inline void addBufferToDelete(GLuint buffer) {
        std::lock_guard<std::mutex> lock(gc_mutex);
        buffers_to_delete.push_back(buffer);
    }

    inline void flush() {
        std::lock_guard<std::mutex> lock(gc_mutex);
        if (!arrays_to_delete.empty()) {
            glDeleteVertexArrays(arrays_to_delete.size(), arrays_to_delete.data());
            arrays_to_delete.clear();
        }
        if (!buffers_to_delete.empty()) {
            glDeleteBuffers(buffers_to_delete.size(), buffers_to_delete.data());
            buffers_to_delete.clear();
        }
    }
};

extern GLGlobalContext gl_global_context;