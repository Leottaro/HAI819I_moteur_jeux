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
#include <iostream>
#include <string>
#include <vector>

#include "objects/textures.hpp"
#include "old/ImageBase.h"

class Texture {
   private:
    GLuint m_texture_id;

    std::vector<glm::u8vec4> m_data;
    int m_width, m_height, m_channels;
    bool m_synchronized;

   public:
    Texture(const std::string& _path) {
        unsigned char* data = stbi_load(_path.c_str(), &m_width, &m_height, &m_channels, 4);
        if (!data) {
            std::cerr << "[Texture] Can't charge date from \"" << _path << "\"" << std::endl;
            return;
        }

        m_data = std::vector<glm::u8vec4>(reinterpret_cast<glm::u8vec4*>(data), reinterpret_cast<glm::u8vec4*>(data) + m_width * m_height);
        stbi_image_free(data);

        initShaderData();
    }

    ~Texture() {
        clearShaderData();
    };

    inline const size_t getWidth() const { return m_width; }
    inline const size_t getHeight() const { return m_height; }
    inline const glm::u8vec4& getPixel(size_t u, size_t v) const { return m_data[v * m_width + u]; }
    inline const glm::u8vec4& getPixel(size_t i) const { return m_data[i]; }
    inline const glm::u8vec4& setPixel(size_t u, size_t v) { return m_data[v * m_width + u]; }
    inline const glm::u8vec4& setPixel(size_t i) { return m_data[i]; }

    void savePPM(const std::string& filePath) const {
        std::ofstream f((filePath + ".ppm").c_str(), std::ios::binary);
        if (f.fail())
            return;

        f << "P6\n"
          << m_width << " " << m_height << "\n255" << std::endl;

        // Write pixel data
        std::vector<char> char_data(m_data.size() * 3);
        for (size_t i = 0; i < m_data.size(); i++) {
            const glm::u8vec4& color = m_data[i];
            char_data[i * 3] = static_cast<char>(color.r);
            char_data[i * 3 + 1] = static_cast<char>(color.g);
            char_data[i * 3 + 2] = static_cast<char>(color.b);
        }
        f.write(char_data.data(), char_data.size());

        f.close();
    }

    void savePNG(const std::string& filePath) const {
        stbi_write_png((filePath + ".png").c_str(), m_width, m_height, 4, reinterpret_cast<const unsigned char*>(m_data.data()), 0);
    }

    void saveBMP(const std::string& filePath) const {
        stbi_write_bmp((filePath + ".bmp").c_str(), m_width, m_height, 4, reinterpret_cast<const unsigned char*>(m_data.data()));
    }

    void saveTGA(const std::string& filePath) const {
        stbi_write_tga((filePath + ".tga").c_str(), m_width, m_height, 4, reinterpret_cast<const unsigned char*>(m_data.data()));
    }

    void initShaderData() {
        clearShaderData();

        glGenTextures(1, &m_texture_id);
        glBindTexture(GL_TEXTURE_2D, m_texture_id);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_width, m_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, m_data.data());
    }

    void bind(GLuint slot = 0) const {
        glActiveTexture(GL_TEXTURE0 + slot);
        glBindTexture(GL_TEXTURE_2D, m_texture_id);
    }

    void clearShaderData() {
        if (m_texture_id) {
            glDeleteTextures(1, &m_texture_id);
            m_texture_id = 0;
        }
    }

    void applyTexture(const Texture& tex_in, size_t pos_x, size_t pos_y) {
        const size_t in_w = tex_in.getWidth();
        const size_t in_h = tex_in.getHeight();

        assert(pos_x + in_w <= m_width && pos_y + in_h <= m_height);

        for (size_t y = 0; y < in_h; ++y) {
            const size_t dst_row = (y + pos_y) * m_width;
            const size_t src_row = y * in_w;

            for (size_t x = 0; x < in_w; ++x) {
                m_data[dst_row + (x + pos_x)] = tex_in.getPixel(src_row + x);
            }
        }
    }

    static std::tuple<Texture, Texture, Texture> generateAtlasses() {
        Texture atlas_albedo("ressources/textures/empty_atlas.png");
        Texture atlas_normal("ressources/textures/empty_atlas.png");
        Texture atlas_specular("ressources/textures/empty_atlas.png");

        size_t x = 0;
        size_t y = 0;
        for (const std::string_view c : texture_names) {
            if (c == "air") continue;

            if (x == atlas_dims) {
                x = 0;
                ++y;
            }

            std::string name(c);

            std::string path_albedo = "ressources/textures/albedos/" + name + ".png";
            std::string path_normal = "ressources/textures/normals/" + name + ".png";
            std::string path_specular = "ressources/textures/speculars/" + name + ".png";

            Texture tex_albedo(path_albedo);
            Texture tex_normal(path_normal);
            Texture tex_specular(path_specular);
            atlas_albedo.applyTexture(tex_albedo, x * texture_size, y * texture_size);
            atlas_normal.applyTexture(tex_normal, x * texture_size, y * texture_size);
            atlas_specular.applyTexture(tex_specular, x * texture_size, y * texture_size);
            ++x;
        }
        return {atlas_albedo, atlas_normal, atlas_specular};
    }
};