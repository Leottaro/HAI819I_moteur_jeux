#pragma once

// GLEW
#include <GL/glew.h>

// GLM
#include <glm/ext.hpp>
#include <glm/glm.hpp>
// #define GLM_ENABLE_EXPERIMENTAL
// #include <glm/gtx/string_cast.hpp>

// STB
#include <stb_image.h>
#include <stb_image_write.h>

// USUAL INCLUDES
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "objects/textures.hpp"
#include "src/Helpers.hpp"

inline GLuint gpu_slot{0};

template <uint8_t __NB_CHANNELS, GLint __GL_FORMAT>
class Texture {
public:
    static constexpr uint8_t NB_CHANNELS = __NB_CHANNELS;
    static constexpr GLint GL_FORMAT = __GL_FORMAT;
    using color_t = std::conditional_t<NB_CHANNELS == 1, uint8_t, glm::vec<NB_CHANNELS, uint8_t, glm::packed_highp>>;

private:
    static constexpr std::pair<color_t, color_t> defaultColors() {
        if constexpr (NB_CHANNELS == 1) {
            return {color_t(0), color_t(255)};
        } else if constexpr (NB_CHANNELS == 2) {
            return {color_t(0, 0), color_t(255, 255)};
        } else if constexpr (NB_CHANNELS == 3) {
            return {color_t(0, 0, 0), color_t(255, 0, 255)};
        } else if constexpr (NB_CHANNELS == 4) {
            return {color_t(0, 0, 0, 255), color_t(255, 0, 255, 255)};
        }
    }
    static constexpr color_t COLOR1 = defaultColors().first;
    static constexpr color_t COLOR2 = defaultColors().second;

public:
    static constexpr std::array<Texture, 3> generateAtlasses() {
        std::array<Texture, 3> atlasses{Texture(ATLAS_SIZE, ATLAS_SIZE), Texture(ATLAS_SIZE, ATLAS_SIZE), Texture(ATLAS_SIZE, ATLAS_SIZE)};

        int x = 0;
        int y = 0;
        for (const std::string_view c : texture_names) {
            if (c == "air")
                continue;

            if (x == ATLAS_DIMS) {
                x = 0;
                y++;
            }

            const int x_pos = x * TEXTURE_SIZE;
            const int y_pos = y * TEXTURE_SIZE;

            std::string name(c);

            std::string path_albedo = "ressources/textures/albedos/" + name + ".png";
            std::string path_normal = "ressources/textures/normals/" + name + ".png";
            std::string path_specular = "ressources/textures/speculars/" + name + ".png";

            Texture tex_albedo(path_albedo);
            Texture tex_normal(path_normal);
            Texture tex_specular(path_specular);
            atlasses[0].applyTexture(tex_albedo, x_pos, y_pos);
            atlasses[1].applyTexture(tex_normal, x_pos, y_pos);
            atlasses[2].applyTexture(tex_specular, x_pos, y_pos);
            ++x;
        }
        return atlasses;
    }

private:
    GLuint m_texture_id{0}, m_gpu_slot{0};
    int m_width, m_height;
    std::vector<color_t> m_data;

    inline size_t XYtoI(size_t x, size_t y) const { return y * m_width + x; }

public:
    Texture(Texture&& other) : m_width(other.m_width), m_height(other.m_height), m_data(other.m_data) {
        other.m_texture_id = 0;
    };
    Texture& operator=(Texture&& other) {
        if (this == &other)
            return *this;
        clearShaderData();

        m_width = other.m_width;
        m_height = other.m_height;
        m_data = other.m_data;

        other.m_texture_id = 0;
        return *this;
    };
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;
    ~Texture() { clearShaderData(); };

    Texture() {}
    Texture(int w, int h) : m_width(w), m_height(h), m_data(w * h) {
        for (int y = 0; y < m_height; y += 2) {
            for (int x = 0; x < m_height; x++) {
                setPixel(x, y, x % 2 == 0 ? COLOR1 : COLOR2);
                setPixel(x, y + 1, x % 2 == 0 ? COLOR2 : COLOR1);
            }
        }
    }
    Texture(const std::string& _path) {
        int nb_channels;
        uint8_t* data = stbi_load(_path.c_str(), &m_width, &m_height, &nb_channels, NB_CHANNELS);
        if (!data)
            throw std::runtime_error("[Texture] Can't load data from \"" + _path + "\"");
        m_data = std::vector<color_t>(reinterpret_cast<color_t*>(data), reinterpret_cast<color_t*>(data) + m_width * m_height);
        stbi_image_free(data);
    }

    inline const GLuint getGpuSlot() const { return m_gpu_slot; }
    inline const int getWidth() const { return m_width; }
    inline const int getHeight() const { return m_height; }
    inline const color_t& getPixel(size_t i) const { return m_data[i]; }
    inline const color_t& getPixel(size_t x, size_t y) const { return getPixel(XYtoI(x, y)); }
    inline void setPixel(size_t i, const color_t& _value) { m_data[i] = _value; }
    inline void setPixel(size_t x, size_t y, const color_t& _value) { setPixel(XYtoI(x, y), _value); }

    inline void savePNG(const std::string& filePath) const { stbi_write_png((filePath + ".png").c_str(), m_width, m_height, NB_CHANNELS, reinterpret_cast<const uint8_t*>(m_data.data()), 0); }
    inline void saveBMP(const std::string& filePath) const { stbi_write_bmp((filePath + ".bmp").c_str(), m_width, m_height, NB_CHANNELS, reinterpret_cast<const uint8_t*>(m_data.data())); }
    inline void saveTGA(const std::string& filePath) const { stbi_write_tga((filePath + ".tga").c_str(), m_width, m_height, NB_CHANNELS, reinterpret_cast<const uint8_t*>(m_data.data())); }

    inline void applyTexture(const Texture& tex_in, int pos_x, int pos_y) {
        const int in_w = tex_in.getWidth();
        const int in_h = tex_in.getHeight();
        assert(pos_x + in_w <= m_width && pos_y + in_h <= m_height);
        for (int y = 0; y < in_h; y++) {
            for (int x = 0; x < in_w; x++) {
                setPixel(pos_x + x, pos_y + y, tex_in.getPixel(x, y));
            }
        }
    }

    inline void initShaderData() {
        m_gpu_slot = gpu_slot++;
        glActiveTexture(GL_TEXTURE0 + m_gpu_slot);
        glGenTextures(1, &m_texture_id);
        glBindTexture(GL_TEXTURE_2D, m_texture_id);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_FORMAT, m_width, m_height, 0, GL_FORMAT, GL_UNSIGNED_BYTE, m_data.data());
    }
    inline void clearShaderData() {
        if (m_texture_id) {
            glDeleteTextures(1, &m_texture_id);
            m_texture_id = 0;
        }
    }
};

using GrayScaleTexture = Texture<1, GL_R>;
using RGTexture = Texture<2, GL_RG>;
using RGBTexture = Texture<3, GL_RGB>;
using RGBATexture = Texture<4, GL_RGBA>;

class ShadowMap {
    GLuint m_FBO{0}, m_texture_i{0}, m_texture_gpu_slot{0};
    int m_width, m_height;
    glm::mat4 m_VP;

public:
    ShadowMap(ShadowMap&&) = delete;
    ShadowMap& operator=(ShadowMap&&) = delete;
    ShadowMap(const ShadowMap&) = delete;
    ShadowMap& operator=(const ShadowMap&) = delete;
    ~ShadowMap() {
        clearShaderData();
    };

    ShadowMap() {}
    ShadowMap(int w, int h) : m_width(w), m_height(h) {}

    inline const GLuint getGpuSlot() const { return m_texture_gpu_slot; }
    inline const glm::mat4& getVP() const { return m_VP; }

    inline bool initShaderData() {
        glGenFramebuffers(1, &m_FBO);

        // Depth texture. Slower than a depth buffer, but you can sample it later in your shader
        m_texture_gpu_slot = gpu_slot++;
        glActiveTexture(GL_TEXTURE0 + m_texture_gpu_slot);
        glGenTextures(1, &m_texture_i);
        glBindTexture(GL_TEXTURE_2D, m_texture_i);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, m_width, m_height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

        constexpr float bordercolor[] = {1.f, 1.f, 1.f, 1.f};
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, bordercolor);

        glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);
        glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, m_texture_i, 0);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE) {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            return true;
        } else {
            std::cout << "PROBLEM IN FBO FBO_ShadowMap::allocate() : FBO NOT successfully created" << std::endl;
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            return false;
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
    inline void bind(const glm::mat4& _VP) {
        m_VP = _VP;
        glViewport(0, 0, m_width, m_height);
        glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);
        glClear(GL_DEPTH_BUFFER_BIT);
    }
    inline void savePNG(const std::string& _filename) const {
        glBindFramebuffer(GL_FRAMEBUFFER, m_FBO); // ensure it's bound
        // No glReadBuffer needed for depth
        std::vector<float> pixels(m_width * m_height);
        glReadPixels(0, 0, m_width, m_height, GL_DEPTH_COMPONENT, GL_FLOAT, pixels.data());

        std::vector<unsigned char> char_data(pixels.size());
        for (size_t i = 0; i < pixels.size(); i++)
            char_data[i] = static_cast<unsigned char>(pixels[i] * 255.0f);
        stbi_write_png((_filename + ".png").c_str(), m_width, m_height, 1, char_data.data(), m_width);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
    inline void clearShaderData() {
        if (m_FBO) {
            glDeleteFramebuffers(1, &m_FBO);
            m_FBO = 0;
        }
    }
};