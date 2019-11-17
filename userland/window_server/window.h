#pragma once

#include <liim/pointers.h>
#include <graphics/rect.h>

class PixelBuffer;

class Window {
public:
    Window(const Rect& rect);
    ~Window();

    Rect rect() const { return m_rect; }

    SharedPtr<PixelBuffer> buffer() { return m_buffer; }
    const SharedPtr<PixelBuffer> buffer() const { return m_buffer; }

private:
    Rect m_rect;
    SharedPtr<PixelBuffer> m_buffer;
};