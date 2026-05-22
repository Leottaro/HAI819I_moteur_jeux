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

class Texture {
   private:
    GLuint m_texture_id{0};

    std::vector<MathHelpers::u8pvec4> m_data;
    int m_width, m_height, m_channels;

   public:
    Texture(const std::string& _path) {
        unsigned char* data = stbi_load(_path.c_str(), &m_width, &m_height, &m_channels, 4);
        if (!data) {
            throw std::runtime_error("[Texture] Can't charge date from \"" + _path + "\"");
        }

        m_data = std::vector<MathHelpers::u8pvec4>(reinterpret_cast<MathHelpers::u8pvec4*>(data), reinterpret_cast<MathHelpers::u8pvec4*>(data) + m_width * m_height);
        stbi_image_free(data);

        initShaderData();
    }

    Texture(const uint64_t w, uint64_t h) : m_width(w), m_height(h), m_channels(4) {
        m_data.resize(w * h);
        const MathHelpers::u8pvec4 black(0, 0, 0, 255);
        const MathHelpers::u8pvec4 purple(255, 0, 255, 255);
        for (int y = 0; y < m_height; y += 2) {
            int y1_index = y * w;
            int y2_index = (y + 1) * w;
            for (int x = 0; x < m_height; ++x) {
                m_data[y1_index + x] = x & 1 ? black : purple;
                m_data[y2_index + x] = x & 1 ? purple : black;
            }
        }
    }

    ~Texture() {
        clearShaderData();
    };

    inline const size_t getWidth() const { return m_width; }
    inline const size_t getHeight() const { return m_height; }
    inline const MathHelpers::u8pvec4& getPixel(size_t u, size_t v) const { return m_data[v * m_width + u]; }
    inline const MathHelpers::u8pvec4& getPixel(size_t i) const { return m_data[i]; }
    inline const MathHelpers::u8pvec4& setPixel(size_t u, size_t v) { return m_data[v * m_width + u]; }
    inline const MathHelpers::u8pvec4& setPixel(size_t i) { return m_data[i]; }

    void savePPM(const std::string& filePath) const {
        std::ofstream f((filePath + ".ppm").c_str(), std::ios::binary);
        if (f.fail())
            return;

        f << "P6\n"
          << m_width << " " << m_height << "\n255" << std::endl;

        // Write pixel data
        std::vector<char> char_data(m_data.size() * 3);
        for (size_t i = 0; i < m_data.size(); i++) {
            const MathHelpers::u8pvec4& color = m_data[i];
            char_data[i * 3] = static_cast<char>(color.r);
            char_data[i * 3 + 1] = static_cast<char>(color.g);
            char_data[i * 3 + 2] = static_cast<char>(color.b);
        }
        f.write(char_data.data(), char_data.size());

        f.close();
    }

    inline void savePNG(const std::string& filePath) const {
        stbi_write_png((filePath + ".png").c_str(), m_width, m_height, 4, reinterpret_cast<const unsigned char*>(m_data.data()), 0);
    }

    inline void saveBMP(const std::string& filePath) const {
        stbi_write_bmp((filePath + ".bmp").c_str(), m_width, m_height, 4, reinterpret_cast<const unsigned char*>(m_data.data()));
    }

    inline void saveTGA(const std::string& filePath) const {
        stbi_write_tga((filePath + ".tga").c_str(), m_width, m_height, 4, reinterpret_cast<const unsigned char*>(m_data.data()));
    }

    inline void initShaderData() {
        clearShaderData();

        glGenTextures(1, &m_texture_id);
        glBindTexture(GL_TEXTURE_2D, m_texture_id);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_width, m_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, m_data.data());
    }

    inline void bind(GLuint slot = 0) const {
        glActiveTexture(GL_TEXTURE0 + slot);
        glBindTexture(GL_TEXTURE_2D, m_texture_id);
    }

    inline void clearShaderData() {
        if (m_texture_id) {
            glDeleteTextures(1, &m_texture_id);
            m_texture_id = 0;
        }
    }

    inline void applyTexture(const Texture& tex_in, size_t pos_x, size_t pos_y) {
        const size_t in_w = tex_in.getWidth();
        const size_t in_h = tex_in.getHeight();

        assert(pos_x + in_w <= size_t(m_width) && pos_y + in_h <= size_t(m_height));

        for (size_t y = 0; y < in_h; ++y) {
            const size_t dst_row = (y + pos_y) * m_width;
            const size_t src_row = y * in_w;

            for (size_t x = 0; x < in_w; ++x) {
                m_data[dst_row + (x + pos_x)] = tex_in.getPixel(src_row + x);
            }
        }
    }

    static constexpr std::tuple<Texture, Texture, Texture> generateAtlasses() {
        Texture atlas_albedo(ATLAS_SIZE, ATLAS_SIZE);
        Texture atlas_normal(ATLAS_SIZE, ATLAS_SIZE);
        Texture atlas_specular(ATLAS_SIZE, ATLAS_SIZE);

        size_t x = 0;
        size_t y = 0;
        for (const std::string_view c : texture_names) {
            if (c == "air")
                continue;

            if (x == ATLAS_DIMS) {
                x = 0;
                ++y;
            }

            const size_t x_pos = x * TEXTURE_SIZE;
            const size_t y_pos = y * TEXTURE_SIZE;

            std::string name(c);

            std::string path_albedo = "ressources/textures/albedos/" + name + ".png";
            std::string path_normal = "ressources/textures/normals/" + name + ".png";
            std::string path_specular = "ressources/textures/speculars/" + name + ".png";

            Texture tex_albedo(path_albedo);
            Texture tex_normal(path_normal);
            Texture tex_specular(path_specular);
            atlas_albedo.applyTexture(tex_albedo, x_pos, y_pos);
            atlas_normal.applyTexture(tex_normal, x_pos, y_pos);
            atlas_specular.applyTexture(tex_specular, x_pos, y_pos);
            ++x;
        }
        return {atlas_albedo, atlas_normal, atlas_specular};
    }
};