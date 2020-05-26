#include <graphics/color.h>
#include <graphics/pixel_buffer.h>
#include <stdio.h>
#include <string.h>

void PixelBuffer::put_pixel(int x, int y, uint32_t p) {
    if (x < 0 || y < 0 || x >= width() || y >= height()) {
        return;
    }
    m_pixels[y * m_width + x] = p;
}

void PixelBuffer::put_pixel(int x, int y, Color color) {
    put_pixel(x, y, color.color());
}

void PixelBuffer::clear() {
    memset(m_pixels, 0, sizeof(uint32_t) * width() * height());
}
