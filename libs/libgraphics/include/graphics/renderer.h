#pragma once

#include <assert.h>
#include <graphics/color.h>
#include <graphics/font.h>
#include <graphics/pixel_buffer.h>
#include <graphics/rect.h>
#include <liim/pointers.h>
#include <liim/string.h>

class Color;

class Renderer {
public:
    Renderer(PixelBuffer& buffer) : m_pixels(buffer) {}

    Color color() const { return m_color; }
    void set_color(Color c) { m_color = c; }

    void fill_rect(const Rect& rect) { fill_rect(rect.x(), rect.y(), rect.width(), rect.height()); }
    void draw_rect(const Rect& rect) { draw_rect(rect.x(), rect.y(), rect.width(), rect.height()); }

    void fill_rect(int x, int y, int width, int height);
    void draw_rect(int x, int y, int width, int height);

    void render_text(int x, int y, const String& text, const Font& font = Font::default_font());

    void fill_circle(int x, int y, int r);
    void draw_circle(int x, int y, int r);

    PixelBuffer& pixels() { return m_pixels; }
    const PixelBuffer& pixels() const { return m_pixels; }

private:
    Color m_color;
    PixelBuffer& m_pixels;
};