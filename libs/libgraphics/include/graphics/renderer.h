#pragma once

#include <assert.h>
#include <liim/string.h>
#include <memory>

#include <graphics/color.h>
#include <graphics/font.h>
#include <graphics/pixel_buffer.h>

class Color;

class Renderer {
public:
    Renderer(std::shared_ptr<PixelBuffer> buffer)
        : m_pixels(buffer) {
        assert(m_pixels);
    }

    Color color() const {
        return m_color;
    }
    void set_color(Color c) {
        m_color = c;
    }

    void fill_rect(int x, int y, int width, int height);
    void draw_rect(int x, int y, int width, int height);

    void render_text(int x, int y, const String& text);

    void fill_circle(int x, int y, int r);
    void draw_circle(int x, int y, int r);

    std::shared_ptr<PixelBuffer> pixels() {
        return m_pixels;
    }
    const std::shared_ptr<PixelBuffer> pixels() const {
        return m_pixels;
    }

    Font& font() {
        return m_font;
    }
    const Font& font() const {
        return m_font;
    }

private:
    Color m_color;
    std::shared_ptr<PixelBuffer> m_pixels;
    Font m_font;
};