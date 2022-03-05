#include <assert.h>
#include <graphics/font.h>
#include <graphics/renderer.h>
#include <liim/scope_guard.h>
#include <liim/utf8_view.h>
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
    draw_line({ x, y }, { x + width - 1, y }, color);
    draw_line({ x, y + height - 1 }, { x + width - 1, y + height - 1 }, color);
    draw_line({ x, y + 1 }, { x, y + height - 2 }, color);
    draw_line({ x + width - 1, y + 1 }, { x + width - 1, y + height - 2 }, color);
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

    auto x_start = translated_start.x();
    auto x_end = translated_end.x();

    auto y_start = translated_start.y();
    auto y_end = translated_end.y();

    auto raw_color = color.color();
    if (x_start == x_end && y_start == y_end) {
        m_pixels.put_pixel(x_start, y_start, raw_color);
        return;
    }

    // Horizontal line fast path
    if (y_start == y_end) {
        if (x_start > x_end) {
            swap(x_start, x_end);
        }

        auto rect = Rect { x_start, y_start, x_end - x_start + 1, 1 };
        auto constrained_rect = constrain(rect);
        if (constrained_rect.empty()) {
            return;
        }

        auto raw_pixels = m_pixels.pixels() + y_start * m_pixels.width();
        for (auto x = constrained_rect.left(); x < constrained_rect.right(); x++) {
            raw_pixels[x] = raw_color;
        }
        return;
    }

    // Vertical line fast path
    if (x_start == x_end) {
        if (y_start > y_end) {
            swap(y_start, y_end);
        }

        auto rect = Rect { x_start, y_start, 1, y_end - y_start + 1 };
        auto constrained_rect = constrain(rect);
        if (constrained_rect.empty()) {
            return;
        }

        auto raw_pixels = m_pixels.pixels() + x_start;
        auto raw_pixels_width = m_pixels.width();
        auto adjusted_y_end = constrained_rect.bottom() * raw_pixels_width;
        for (auto y = constrained_rect.top() * raw_pixels_width; y < adjusted_y_end; y += raw_pixels_width) {
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

void Renderer::render_text(const String& text, const Rect& rect, Color color, TextAlign align, Font& font) {
    auto old_clip_rect = m_bounding_rect;
    auto old_translation = m_translation;
    auto restorer = ScopeGuard { [&] {
        m_bounding_rect = old_clip_rect;
        m_translation = old_translation;
    } };
    m_bounding_rect = old_clip_rect.intersection_with(translate(rect));
    m_translation = m_bounding_rect.top_left();

    auto font_metrics = font.font_metrics();

    auto lines = text.split_view('\n');
    int text_height = lines.size() * font_metrics.line_height();

    int start_y = 0;
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

    for (auto& line_text : lines) {
        int text_width = 0;

        auto utf8_view = Utf8View { line_text };
        auto glyphs = Vector<Glyph> {};
        for (auto code_point : utf8_view) {
            auto glyph_id = font.glyph_id_for_code_point(code_point);
            if (!glyph_id) {
                glyph_id = font.fallback_glyph_id();
            }
            // We really should have a fallback glyph id in some font.
            assert(glyph_id);

            auto glyph_metrics = font.glyph_metrics(*glyph_id);
            text_width += glyph_metrics.advance_width();
            glyphs.add(Glyph { *glyph_id, move(glyph_metrics) });
        }

        int start_x = 0;
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

        for (auto& glyph : glyphs) {
            // FIXME: use a glyph atlas
            auto bitmap = font.rasterize_glyph(glyph.id(), color);

            // FIXME: start_x and start_y should be adjusted by the glyph metrics somehow.
            draw_bitmap(*bitmap, bitmap->rect(), { start_x, start_y, bitmap->width(), bitmap->height() });

            start_x += glyph.metrics().advance_width();
        }

        start_y += font_metrics.line_height();
    }
}

void Renderer::set_bounding_rect(const Rect& rect) {
    m_bounding_rect = rect.intersection_with(m_pixels.rect());
    m_translation = m_bounding_rect.top_left();
}

void Renderer::set_translation(const Point& translation) {
    m_translation = m_translation.translated(translation);
}
