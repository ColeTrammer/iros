#pragma once

#include <assert.h>
#include <pointers.h>

#include <graphics/color.h>
#include <graphics/pixel_buffer.h>

class Color;

class Renderer {
public:
    Renderer(SharedPtr<PixelBuffer> buffer)
        : m_pixels(buffer)
    {
        assert(m_pixels);
    }

    Color color() const { return m_color; }
    void set_color(Color c) { m_color = c; }

    void fill_rect(int x, int y, int width, int height);

private:
    Color m_color;
    SharedPtr<PixelBuffer> m_pixels;
};