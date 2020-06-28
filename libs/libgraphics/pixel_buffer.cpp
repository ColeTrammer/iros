#include <graphics/color.h>
#include <graphics/pixel_buffer.h>
#include <stdio.h>
#include <string.h>

void PixelBuffer::clear() {
    memset(m_pixels, 0, sizeof(uint32_t) * width() * height());
}

void PixelBuffer::clear_after_y(int y) {
    memset(m_pixels + y * width(), 0, sizeof(uint32_t) * width() * (height() - y));
}

void PixelBuffer::shrink_width(int new_width) {
    auto* pixels = this->pixels();
    for (auto y = 1; y < height(); y++) {
        for (auto x = 0; x < new_width; x++) {
            pixels[y * new_width + x] = pixels[y * width() + x];
        }
    }

    m_width = new_width;
}

void PixelBuffer::adjust_for_size_change(int old_width, int old_height) {
    auto background_color = Color(ColorValue::Black).color();
    for (auto yi = height(); yi > 0; yi--) {
        auto new_y = yi - 1;
        for (auto xi = width(); xi > 0; xi--) {
            auto new_x = xi - 1;
            if (new_y >= old_height || new_x >= old_width) {
                put_pixel(new_x, new_y, background_color);
                continue;
            }

            auto old_value = m_pixels[new_y * old_width + new_x];
            put_pixel(new_x, new_y, old_value);
        }
    }
}
