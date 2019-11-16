#include <graphics/renderer.h>

void Renderer::fill_rect(int x, int y, int width, int height)
{
    for (int i = x; i < x + width; i++) {
        for (int j = y; j < y + height; j++) {
            m_pixels->put_pixel(i, j, color());
        }
    }
}

void Renderer::render_text(int x, int y, const String& text)
{
    for (int k = 0; k < text.size(); k++) {
        for (int i = 0; i < 16; i++) {
            for (int j = 0; j < 8; j++) {
                m_pixels->put_pixel(k * 8 + x - j + 8, y + i, font().get_for_character(text[k])->get(i * 8 + j) ? color().color() : 0xFF000000);
            }
        }
    }
}