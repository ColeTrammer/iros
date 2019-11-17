#include <graphics/renderer.h>

void Renderer::fill_rect(int x, int y, int width, int height)
{
    for (int i = x; i < x + width; i++) {
        for (int j = y; j < y + height; j++) {
            m_pixels->put_pixel(i, j, color());
        }
    }
}

void Renderer::draw_rect(int x, int y, int width, int height)
{
    for (int r = x; r < x + width; r++) {
        m_pixels->put_pixel(r, y, color());
        m_pixels->put_pixel(r, y + height, color());
    }

    for (int c = y + 1; c < y + height; c++) {
        m_pixels->put_pixel(x, c, color());
        m_pixels->put_pixel(x + width, c, color());
    }
}

void Renderer::fill_circle(int x, int y, int r)
{
    int r2 = r * r;
    for (int a = x - r; a < x + r; a++) {
        for (int b = y - r; b <= y + r; b++) {
            int da = a - x;
            int db = b - y;
            int dd = da * da + db * db;
            if (dd <= r2) {
                m_pixels->put_pixel(a, b, color());
            }
        }
    }
}

void Renderer::draw_circle(int x, int y, int r)
{
    int r2 = r * r;
    for (int a = x - r; a < x + r; a++) {
        for (int b = y - r; b < y + r; b++) {
            int da = a - x;
            int db = b - y;
            int dd = da * da + db * db;
            if (abs(dd - r2) <= r) {
                m_pixels->put_pixel(a, b, color());
            }
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