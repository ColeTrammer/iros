#pragma once

#include <assert.h>
#include <graphics/color.h>
#include <graphics/rect.h>
#include <liim/pointers.h>
#include <stdint.h>
#include <stdio.h>

class Bitmap {
public:
    Bitmap(bool has_alpha) : m_should_deallocate(false), m_has_alpha(has_alpha) {}

    Bitmap(int width, int height, bool has_alpha) : Bitmap(new uint32_t[width * height], width, height, has_alpha) {
        m_should_deallocate = true;
    }

    Bitmap(const Rect& rect, bool has_alpha) : Bitmap(rect.width(), rect.height(), has_alpha) {}

    ~Bitmap() {
        if (m_should_deallocate && m_pixels) {
            delete[] m_pixels;
            m_pixels = nullptr;
        }
    }

    static SharedPtr<Bitmap> wrap(uint32_t* pixels, int width, int height, bool has_alpha) {
        return make_shared<Bitmap>(pixels, width, height, has_alpha);
    }

    int width() const { return m_width; }
    int height() const { return m_height; }
    int size_in_bytes() const { return m_width * m_height * sizeof(uint32_t); }

    uint32_t* pixels() { return m_pixels; }
    const uint32_t* pixels() const { return m_pixels; }

    void clear(Color color = ColorValue::Black);
    void clear_after_y(int y, Color color = ColorValue::Black);
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

    bool has_alpha() const { return m_has_alpha; }

    Bitmap(uint32_t* pixels, int width, int height, bool has_alpha)
        : m_has_alpha(has_alpha), m_width(width), m_height(height), m_pixels(pixels) {}

private:
    bool m_should_deallocate { false };
    bool m_has_alpha { false };
    int m_width { 0 };
    int m_height { 0 };
    uint32_t* m_pixels { nullptr };
};
