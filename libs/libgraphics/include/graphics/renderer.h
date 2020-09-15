#pragma once

#include <assert.h>
#include <graphics/color.h>
#include <graphics/font.h>
#include <graphics/pixel_buffer.h>
#include <graphics/rect.h>
#include <graphics/text_align.h>
#include <liim/pointers.h>
#include <liim/string.h>

class Renderer {
public:
    Renderer(PixelBuffer& buffer) : m_pixels(buffer) {}

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

    void draw_bitmap(const PixelBuffer& src, const Rect& src_rect, const Rect& dest_rect);

    PixelBuffer& pixels() { return m_pixels; }
    const PixelBuffer& pixels() const { return m_pixels; }

private:
    PixelBuffer& m_pixels;
};
