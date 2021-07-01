#include <assert.h>
#include <graphics/font.h>
#include <graphics/renderer.h>
#include <math.h>
#include <stdlib.h>

static constexpr Color alpha_blend(Color foreground, Color background, bool bg_opaque = false) {
    auto alpha_a = foreground.a();

    if (alpha_a == 0) {
        return background;
    } else if (alpha_a == 0xFF) {
        return foreground;
    }

    if (bg_opaque) {
        auto r_out = ((foreground.r() * alpha_a) + background.r() * (255U - alpha_a)) / 255U;
        auto g_out = ((foreground.g() * alpha_a) + background.g() * (255U - alpha_a)) / 255U;
        auto b_out = ((foreground.b() * alpha_a) + background.b() * (255U - alpha_a)) / 255U;
        return Color(r_out, g_out, b_out, 255U);
    }

    auto alpha_b = background.a();
    auto a_out = alpha_a + alpha_b * (255U - alpha_a) / 255U;

    auto scaled_r_a = foreground.r() * alpha_a;
    auto scaled_r_b = background.r() * alpha_b * (255U - alpha_a) / 255U;
    auto r_out = (scaled_r_a + scaled_r_b) / (a_out);

    auto scaled_g_a = foreground.g() * alpha_a;
    auto scaled_g_b = background.g() * alpha_b * (255U - alpha_a) / 255U;
    auto g_out = (scaled_g_a + scaled_g_b) / (a_out);

    auto scaled_b_a = foreground.b() * alpha_a;
    auto scaled_b_b = background.b() * alpha_b * (255U - alpha_a) / 255U;
    auto b_out = (scaled_b_a + scaled_b_b) / (a_out);

    return Color(r_out, g_out, b_out, a_out);
}

static_assert(alpha_blend(ColorValue::Black, ColorValue::Black) == ColorValue::Black);
static_assert(alpha_blend(Color(255, 255, 255, 0), ColorValue::Black) == ColorValue::Black);
static_assert(alpha_blend(Color(255, 255, 255, 200), ColorValue::Black).a() == 255);
static_assert(alpha_blend(Color(140, 0, 0, 200), ColorValue::Black) == Color(109, 0, 0, 255));
static_assert(alpha_blend(Color(120, 0, 0, 160), Color(39, 0, 0, 40)) == Color(113, 0, 0, 174));
static_assert(alpha_blend(ColorValue::Clear, ColorValue::Clear) == ColorValue::Clear);

void Renderer::fill_rect(int x, int y, int width, int height, Color color_object) {
    if (color_object.a() == 0) {
        return;
    }
    if (color_object.a() == 0xFF) {
        clear_rect(x, y, width, height, color_object);
        return;
    }

    auto& buffer = m_pixels;

    auto translated_x = translate_x(x);
    auto translated_y = translate_y(y);

    auto x_start = constrain_x(translated_x);
    auto x_end = constrain_x(translated_x + width);

    auto y_start = constrain_y(translated_y);
    auto y_end = constrain_y(translated_y + height);

    if (y_end <= y_start || x_end <= x_start) {
        return;
    }

    bool bg_opaque = !buffer.has_alpha();
    for (int y = y_start; y < y_end; y++) {
        auto* base = buffer.pixels() + (y * buffer.width());
        for (int x = x_start; x < x_end; x++) {
            base[x] = alpha_blend(color_object, base[x], bg_opaque).color();
        }
    }
}

void Renderer::draw_rect(int x, int y, int width, int height, Color color) {
    auto translated_x = translate_x(x);
    auto translated_y = translate_y(y);

    auto left = constrain_x(translated_x);
    auto right = constrain_x(translated_x + width);

    auto top = constrain_y(translated_y);
    auto bottom = constrain_y(translated_y + height);

    for (int r = left; r < right; r++) {
        m_pixels.put_pixel(r, top, color);
        m_pixels.put_pixel(r, bottom - 1, color);
    }

    for (int c = top + 1; c < bottom - 1; c++) {
        m_pixels.put_pixel(left, c, color);
        m_pixels.put_pixel(right - 1, c, color);
    }
}

void Renderer::clear_rect(int x, int y, int width, int height, Color color_object) {
    auto& buffer = m_pixels;

    auto translated_x = translate_x(x);
    auto translated_y = translate_y(y);

    auto x_start = constrain_x(translated_x);
    auto x_end = constrain_x(translated_x + width);

    auto y_start = constrain_y(translated_y);
    auto y_end = constrain_y(translated_y + height);

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

void Renderer::draw_line(Point start, Point end, Color color) {
    auto translated_start = translate(start);
    auto translated_end = translate(end);

    auto constrained_start = constrain(translated_start);
    auto constrained_end = constrain(translated_end);

    auto x_start = constrained_start.x();
    auto x_end = constrained_end.x();

    auto y_start = constrained_start.y();
    auto y_end = constrained_end.y();

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

    auto fixed_x = constrain_x(translate_x(x));
    auto fixed_y = constrain_y(translate_y(y));

    for (int a = fixed_x - r; a < fixed_x + r; a++) {
        for (int b = fixed_y - r; b <= fixed_y + r; b++) {
            int da = a - fixed_x;
            int db = b - fixed_y;
            int dd = da * da + db * db;
            if (dd <= r2) {
                m_pixels.put_pixel(a, b, color);
            }
        }
    }
}

void Renderer::draw_circle(int x, int y, int r, Color color) {
    int r2 = r * r;

    auto fixed_x = constrain_x(translate_x(x));
    auto fixed_y = constrain_y(translate_y(y));

    for (int a = fixed_x - r; a < fixed_x + r; a++) {
        for (int b = fixed_y - r; b <= fixed_y + r; b++) {
            int da = a - fixed_x;
            int db = b - fixed_y;
            int dd = da * da + db * db;
            if (abs(dd - r2) <= r) {
                m_pixels.put_pixel(a, b, color);
            }
        }
    }
}

void Renderer::draw_bitmap(const Bitmap& src, const Rect& src_rect_in, const Rect& dest_rect_in) {
    assert(src_rect_in.width() == dest_rect_in.width());
    assert(src_rect_in.height() == dest_rect_in.height());

    auto translated_dest_rect = translate(dest_rect_in);
    auto dest_rect = translated_dest_rect.intersection_with(m_bounding_rect);
    if (dest_rect == Rect()) {
        return;
    }

    auto src_rect = Rect {
        src_rect_in.x() + dest_rect.x() - translated_dest_rect.x(),
        src_rect_in.y() + dest_rect.y() - translated_dest_rect.y(),
        src_rect_in.width() + dest_rect.width() - translated_dest_rect.width(),
        src_rect_in.height() + dest_rect.height() - translated_dest_rect.height(),
    };

    auto x_offset = dest_rect.x() - src_rect.x();
    auto y_offset = dest_rect.y() - src_rect.y();

    auto src_x_start = src_rect.x();
    auto src_x_end = src_x_start + src_rect.width();
    auto src_y_start = src_rect.y();
    auto src_y_end = src_y_start + src_rect.height();

    auto src_width = src.width();
    auto raw_src = src.pixels();

    auto raw_dest = m_pixels.pixels();
    auto dest_width = m_pixels.width();

    if (!src.has_alpha()) {
        auto row_width_in_bytes = (src_x_end - src_x_start) * sizeof(uint32_t);
        for (auto src_y = src_y_start; src_y < src_y_end; src_y++) {
            auto dest_y = y_offset + src_y;
            memcpy(raw_dest + dest_y * dest_width + x_offset + src_x_start, raw_src + src_y * src_width + src_x_start, row_width_in_bytes);
        }
    }

    bool bg_opaque = !m_pixels.has_alpha();
    for (auto src_y = src_y_start; src_y < src_y_end; src_y++) {
        auto dest_y = y_offset + src_y;
        for (auto src_x = src_x_start; src_x < src_x_end; src_x++) {
            auto dest_x = x_offset + src_x;
            auto& background = raw_dest[dest_y * dest_width + dest_x];
            auto foreground = raw_src[src_y * src_width + dest_x - x_offset];
            background = alpha_blend(foreground, background, bg_opaque).color();
        }
    }
}

void Renderer::render_text(int x, int y, const String& text, Color color, const Font& font) {
    auto translated_x = translate_x(x);
    auto translated_y = translate_y(y);

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
                    m_pixels.put_pixel(translated_x + x_offset - j + 7, translated_y + y_offset + i, color.color());
                }
            }
        }
    }
}

void Renderer::render_text(const String& text, const Rect& rect_in, Color color, TextAlign align, const Font& font) {
    auto lines = text.split_view('\n');
    int text_height = lines.size() * 16;

    auto rect = translate(rect_in);

    int start_y = rect.y();
    if (text_height < rect.height()) {
        switch (align) {
            case TextAlign::TopLeft:
            case TextAlign::TopCenter:
            case TextAlign::TopRight:
                start_y += 0;
                break;
            case TextAlign::CenterLeft:
            case TextAlign::Center:
            case TextAlign::CenterRight:
                start_y += (rect.height() - text_height) / 2;
                break;
            case TextAlign::BottomLeft:
            case TextAlign::BottomCenter:
            case TextAlign::BottomRight:
                start_y += rect.height() - text_height;
                break;
        }
    }

    auto color_value = color.color();
    int lines_to_render = min(lines.size(), rect.height() / 16);
    for (int l = 0; l < lines_to_render; l++) {
        auto& line_text = lines[l];
        int text_width = line_text.size() * 8;

        int start_x = rect.x();
        if (text_width < rect.width()) {
            switch (align) {
                case TextAlign::TopLeft:
                case TextAlign::CenterLeft:
                case TextAlign::BottomLeft:
                    start_x += 0;
                    break;
                case TextAlign::TopCenter:
                case TextAlign::Center:
                case TextAlign::BottomCenter:
                    start_x += (rect.width() - text_width) / 2;
                    break;
                case TextAlign::TopRight:
                case TextAlign::CenterRight:
                case TextAlign::BottomRight:
                    start_x += rect.width() - text_width;
                    break;
            }
        }

        auto chars_to_render = min(line_text.size(), static_cast<size_t>(rect.width() / 8));
        for (size_t k = 0; k < chars_to_render; k++) {
            char c = line_text[k];
            auto* bitmap = font.get_for_character(c);
            if (!bitmap) {
                bitmap = font.get_for_character('?');
                assert(bitmap);
            }
            for (int i = 0; i < 16; i++) {
                for (int j = 0; j < 8; j++) {
                    if (bitmap->get(i * 8 + j)) {
                        m_pixels.put_pixel(start_x - j + 7, start_y + i, color_value);
                    }
                }
            }

            start_x += 8;
        }

        start_y += 16;
    }
}

void Renderer::set_bounding_rect(const Rect& rect) {
    m_bounding_rect = rect.intersection_with(m_pixels.rect());
}
