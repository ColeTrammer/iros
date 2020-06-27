#include <graphics/font.h>
#include <graphics/renderer.h>
#include <math.h>
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
    auto x_start = clamp(start.x(), 0, m_pixels.width() - 1);
    auto x_end = clamp(end.x(), 0, m_pixels.width() - 1);

    auto y_start = clamp(start.y(), 0, m_pixels.height() - 1);
    auto y_end = clamp(end.y(), 0, m_pixels.height() - 1);

    auto raw_color = color.color();
    if (x_start == x_end && y_start == y_end) {
        m_pixels.put_pixel(x_start, y_start, raw_color);
        return;
    }

    if (y_start == y_end) {
        if (x_start > x_end) {
            swap(x_start, x_end);
        }

        auto raw_pixels = m_pixels.pixels() + y_start * m_pixels.width();

        for (auto x = x_start; x <= x_end; x++) {
            raw_pixels[x] = raw_color;
        }
        return;
    }

    if (x_start == x_end) {
        if (y_start > y_end) {
            swap(y_start, y_end);
        }

        auto raw_pixels = m_pixels.pixels() + x_start;
        auto raw_pixels_width = m_pixels.width();
        auto adjusted_y_end = y_end * raw_pixels_width;
        for (auto y = y_start * raw_pixels_width; y <= adjusted_y_end; y += raw_pixels_width) {
            raw_pixels[y] = raw_color;
        }
        return;
    }

    if (abs(x_end - x_start) >= abs(y_end - y_start)) {
        auto iterations = abs(x_end - x_start) + 1;
        auto x_step = x_end > x_start ? 1 : -1;
        auto y_step = static_cast<float>(y_end - y_start) / static_cast<float>(iterations);
        auto x = x_start;
        auto y = static_cast<float>(y_start);
        for (auto i = 0; i < iterations; i++) {
            m_pixels.put_pixel(x, roundf(y), raw_color);
            x += x_step;
            y += y_step;
        }
        return;
    }

    auto iterations = abs(y_end - y_start) + 1;
    auto x_step = static_cast<float>(x_end - x_start) / static_cast<float>(iterations);
    auto y_step = y_end > y_start ? 1 : -1;
    auto x = static_cast<float>(x_start);
    auto y = y_start;
    for (auto i = 0; i < iterations; i++) {
        m_pixels.put_pixel(roundf(x), y, raw_color);
        x += x_step;
        y += y_step;
    }
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
