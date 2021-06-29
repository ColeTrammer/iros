#pragma once

#include <assert.h>
#include <graphics/bitmap.h>
#include <graphics/color.h>
#include <graphics/font.h>
#include <graphics/rect.h>
#include <graphics/text_align.h>
#include <liim/pointers.h>
#include <liim/string.h>

class Renderer {
public:
    Renderer(Bitmap& buffer) : m_pixels(buffer), m_bounding_rect(buffer.rect()) {}

    void fill_rect(int x, int y, int width, int height, Color color);
    void draw_rect(int x, int y, int width, int height, Color color);
    void clear_rect(int x, int y, int width, int height, Color color);
    void fill_rect(const Rect& rect, Color color) { fill_rect(rect.x(), rect.y(), rect.width(), rect.height(), color); }
    void draw_rect(const Rect& rect, Color color) { draw_rect(rect.x(), rect.y(), rect.width(), rect.height(), color); }
    void clear_rect(const Rect& rect, Color color) { clear_rect(rect.x(), rect.y(), rect.width(), rect.height(), color); }

    void draw_line(Point start, Point end, Color color);

    void fill_circle(int x, int y, int r, Color color);
    void draw_circle(int x, int y, int r, Color color);

    void render_text(int x, int y, const String& text, Color color, const Font& font = Font::default_font());
    void render_text(const String& text, const Rect& rect, Color color, TextAlign text_align = TextAlign::CenterLeft,
                     const Font& font = Font::default_font());

    void draw_bitmap(const Bitmap& src, const Rect& src_rect, const Rect& dest_rect);

    Bitmap& pixels() { return m_pixels; }
    const Bitmap& pixels() const { return m_pixels; }

    void set_bounding_rect(const Rect& rect);

private:
    Rect translate(const Rect& r) const { return r.translated(m_bounding_rect.top_left()); }
    Point translate(const Point& p) const { return p.translated(m_bounding_rect.top_left()); }
    int translate_x(int x) const { return x + m_bounding_rect.left(); }
    int translate_y(int y) const { return y + m_bounding_rect.top(); }

    Point constrain(const Point& p) const { return m_bounding_rect.constrained(p); }
    int constrain_x(int x) const { return clamp(x, m_bounding_rect.left(), m_bounding_rect.right()); }
    int constrain_y(int y) const { return clamp(y, m_bounding_rect.top(), m_bounding_rect.bottom()); }

    Bitmap& m_pixels;
    Rect m_bounding_rect;
};
