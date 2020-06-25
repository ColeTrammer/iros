#include <graphics/color.h>
#include <graphics/pixel_buffer.h>
#include <stdio.h>
#include <string.h>

void PixelBuffer::clear() {
    memset(m_pixels, 0, sizeof(uint32_t) * width() * height());
}
