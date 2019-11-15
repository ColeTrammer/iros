#pragma once

#include <pointers.h>
#include <stdint.h>

class Color;

class PixelBuffer {
public:
    PixelBuffer(int width, int height)
        : PixelBuffer(new uint32_t[width * height], width, height)
    {
        m_should_deallocate = true;
    }

    ~PixelBuffer()
    {
        if (m_should_deallocate) {
            delete[] m_pixels;
        }
        m_pixels = nullptr;
    }

    static SharedPtr<PixelBuffer> wrap(uint32_t* pixels, int width, int height)
    {
        return SharedPtr<PixelBuffer>(new PixelBuffer(pixels, width, height));
    }

    int width() const { return m_width; }
    int height() const { return m_height; }
    int size_in_bytes() const { return m_width * m_height * sizeof(uint32_t); }

    void put_pixel(int x, int y, uint32_t p);
    void put_pixel(int x, int y, Color color);

private:
    PixelBuffer(uint32_t* pixels, int width, int height)
        : m_width(width)
        , m_height(height)
        , m_pixels(pixels)
    {
    }

    bool m_should_deallocate { false };
    int m_width { 0 };
    int m_height { 0 };
    uint32_t* m_pixels { nullptr };
};