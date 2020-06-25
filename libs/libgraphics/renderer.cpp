#include <graphics/font.h>
#include <graphics/renderer.h>
#include <stdlib.h>

void Renderer::fill_rect(int x, int y, int width, int height, Color color_object) {
    auto& buffer = m_pixels;

    auto y_start = LIIM::max(0, y);
    auto y_end = LIIM::min(y + height, buffer.height());

    auto x_start = LIIM::max(0, x);
    auto x_end = LIIM::min(x + width, buffer.width());

    if (y_end <= y_start || x_end <= x_start) {
        return;
    }

    auto color = color_object.color();
    for (int y = y_start; y < y_end; y++) {
        auto* base = buffer.pixels() + (y * buffer.width());
        for (int x = x_start; x < x_end; x++) {
            base[x] = color;
        }
    }
}

void Renderer::draw_rect(int x, int y, int width, int height, Color color) {
    for (int r = x; r < x + width; r++) {
        m_pixels.put_pixel(r, y, color);
        m_pixels.put_pixel(r, y + height - 1, color);
    }

    for (int c = y + 1; c < y + height - 1; c++) {
        m_pixels.put_pixel(x, c, color);
        m_pixels.put_pixel(x + width - 1, c, color);
    }
}

void Renderer::draw_line(Point start, Point end, Color color) {
    auto x_start = max(0, min(start.x(), end.x()));
    auto x_end = min(m_pixels.width(), max(start.x(), end.x()));

    auto y_start = max(0, min(start.y(), end.y()));
    auto y_end = min(m_pixels.height(), max(start.y(), end.y()));

    if (x_start == x_end && y_start == y_end) {
        return;
    }

    auto raw_color = color.color();
    if (y_start == y_end) {
        auto raw_pixels = m_pixels.pixels() + y_start * m_pixels.width();
        for (auto x = x_start; x < x_end; x++) {
            raw_pixels[x] = raw_color;
        }
        return;
    }

    if (x_start == x_end) {
        auto raw_pixels = m_pixels.pixels() + x_start;
        auto raw_pixels_width = m_pixels.width();
        auto adjusted_y_end = y_end * raw_pixels_width;
        for (auto y = y_start * raw_pixels_width; y < adjusted_y_end; y += raw_pixels_width) {
            raw_pixels[y] = raw_color;
        }
        return;
    }

    assert(false);
}

void Renderer::fill_circle(int x, int y, int r, Color color) {
    int r2 = r * r;
    for (int a = x - r; a < x + r; a++) {
        for (int b = y - r; b <= y + r; b++) {
            int da = a - x;
            int db = b - y;
            int dd = da * da + db * db;
            if (dd <= r2) {
                m_pixels.put_pixel(a, b, color);
            }
        }
    }
}

void Renderer::draw_circle(int x, int y, int r, Color color) {
    int r2 = r * r;
    for (int a = x - r; a < x + r; a++) {
        for (int b = y - r; b < y + r; b++) {
            int da = a - x;
            int db = b - y;
            int dd = da * da + db * db;
            if (abs(dd - r2) <= r) {
                m_pixels.put_pixel(a, b, color);
            }
        }
    }
}

void Renderer::render_text(int x, int y, const String& text, Color color, const Font& font) {
    int x_offset = -8;
    int y_offset = 0;
    for (size_t k = 0; k < text.size(); k++) {
        char c = text[k];
        if (c == '\n') {
            y_offset += 16;
            x_offset = -8;
            continue;
        } else {
            x_offset += 8;
        }

        auto* bitmap = font.get_for_character(c);
        if (!bitmap) {
            bitmap = font.get_for_character('?');
            assert(bitmap);
        }
        for (int i = 0; i < 16; i++) {
            for (int j = 0; j < 8; j++) {
                if (bitmap->get(i * 8 + j)) {
                    m_pixels.put_pixel(x + x_offset - j + 7, y + y_offset + i, color.color());
                }
            }
        }
    }
}
