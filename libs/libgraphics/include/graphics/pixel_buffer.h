#pragma once

#include <assert.h>
#include <graphics/color.h>
#include <graphics/rect.h>
#include <liim/pointers.h>
#include <stdint.h>
#include <stdio.h>

class PixelBuffer {
public:
    PixelBuffer() { m_should_deallocate = false; }

    PixelBuffer(int width, int height) : PixelBuffer(new uint32_t[width * height], width, height) { m_should_deallocate = true; }

    PixelBuffer(const Rect& rect) : PixelBuffer(rect.width(), rect.height()) {}

    ~PixelBuffer() {
        if (m_should_deallocate && m_pixels) {
            delete[] m_pixels;
            m_pixels = nullptr;
        }
    }

    static SharedPtr<PixelBuffer> wrap(uint32_t* pixels, int width, int height) { return make_shared<PixelBuffer>(pixels, width, height); }

    int width() const { return m_width; }
    int height() const { return m_height; }
    int size_in_bytes() const { return m_width * m_height * sizeof(uint32_t); }

    uint32_t* pixels() { return m_pixels; }
    const uint32_t* pixels() const { return m_pixels; }

    void clear();
    void clear_after_y(int y);
    void shrink_width(int new_width);
    void adjust_for_size_change(int old_width, int old_height);

    void __attribute__((always_inline)) put_pixel(int x, int y, uint32_t p) {
        if (x < 0 || y < 0 || x >= width() || y >= height()) {
            return;
        }
        m_pixels[y * m_width + x] = p;
    }
    void __attribute__((always_inline)) put_pixel(int x, int y, Color color) { put_pixel(x, y, color.color()); }

    uint32_t get_pixel(int x, int y) const {
        assert(m_pixels);
        return m_pixels[y * m_width + x];
    }

    PixelBuffer(uint32_t* pixels, int width, int height) : m_width(width), m_height(height), m_pixels(pixels) {}

private:
    bool m_should_deallocate { false };
    int m_width { 0 };
    int m_height { 0 };
    uint32_t* m_pixels { nullptr };
};
