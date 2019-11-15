#include <graphics/renderer.h>

void Renderer::fill_rect(int x, int y, int width, int height)
{
    for (int i = x; i < x + width; i++) {
        for (int j = y; j < y + height; j++) {
            m_pixels->put_pixel(i, j, color());
        }
    }
}