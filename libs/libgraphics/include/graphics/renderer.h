#pragma once

#include <assert.h>
#include <liim/pointers.h>
#include <liim/string.h>

#include <graphics/color.h>
#include <graphics/font.h>
#include <graphics/pixel_buffer.h>

class Color;

class Renderer {
public:
    Renderer(PixelBuffer& buffer) : m_pixels(buffer) {}

    Color color() const { return m_color; }
    void set_color(Color c) { m_color = c; }

    void fill_rect(int x, int y, int width, int height);
    void draw_rect(int x, int y, int width, int height);

    void render_text(int x, int y, const String& text);

    void fill_circle(int x, int y, int r);
    void draw_circle(int x, int y, int r);

    PixelBuffer& pixels() { return m_pixels; }
    const PixelBuffer& pixels() const { return m_pixels; }

    Font& font() { return m_font; }
    const Font& font() const { return m_font; }

private:
    Color m_color;
    PixelBuffer& m_pixels;
    Font m_font;
};