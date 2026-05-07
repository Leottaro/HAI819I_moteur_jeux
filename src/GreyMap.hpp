#pragma once

// GLM
// #include <glm/ext.hpp>
#include <glm/glm.hpp>

// STB
#include <stb_image.h>
#include <stb_image_write.h>

// USUAL INCLUDES
#include <stdint.h>

#include <stdexcept>
#include <string>
#include <vector>

class GreyMap {
   private:
    size_t m_width, m_height;
    std::vector<uint8_t> m_data;

    inline size_t index(size_t x, size_t y) const {
        return y * m_width + x;
    }

   public:
    GreyMap(size_t _width, size_t _height, size_t _val) : m_width(_width), m_height(_height) { m_data.assign(m_width * m_height, _val); }
    GreyMap(const glm::u64vec2 _img_size, size_t _val) : m_width(_img_size.x), m_height(_img_size.y) { m_data.assign(m_width * m_height, _val); }
    GreyMap(const GreyMap* _old_map) : m_width(_old_map->m_width), m_height(_old_map->m_height), m_data(_old_map->m_data) {}
    GreyMap(const std::string& _path) {
        int width, height;
        uint8_t* data = stbi_load(_path.c_str(), &width, &height, nullptr, 1);

        if (!data) {
            throw std::runtime_error("[Texture] Can't charge date from \"" + _path + "\"");
        }

        m_width = width;
        m_height = height;
        m_data.assign(data, data + m_width * m_height);

        stbi_image_free(data);
    }

    inline uint8_t& getPixel(size_t i) { return m_data[i]; }
    inline const uint8_t& getPixel(size_t i) const { return m_data[i]; }
    inline uint8_t& getPixel(size_t x, size_t y) { return m_data[index(x, y)]; }
    inline const uint8_t& getPixel(size_t x, size_t y) const { return m_data[index(x, y)]; }
    inline const uint8_t& getPixelSafe(size_t x, size_t y) const { return m_data[(y % m_height) * m_width + (x % m_width)]; }

    inline const size_t getWidth() const { return m_width; }
    inline const size_t getHeight() const { return m_height; }

    GreyMap submap(const GreyMap& _map_in, glm::u64vec2 _start, glm::u64vec2 _end) {
        glm::u64vec2 size = _end - _start;

        GreyMap map_out(size, 0);

        for (uint64_t y = 0; y < size.y; ++y) {
            for (uint64_t x = 0; x < size.x; ++x) {
                uint64_t src_x = (_start.x + x) % _map_in.getWidth();
                uint64_t src_y = (_start.y + y) % _map_in.getHeight();

                map_out.getPixel(x, y) = _map_in.getPixel(src_x, src_y);
            }
        }

        return map_out;
    }
};