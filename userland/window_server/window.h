#pragma once

#include <liim/pointers.h>
#include <liim/string.h>
#include <graphics/rect.h>

class PixelBuffer;

class Window {
public:
    Window(const String& shm_path, const Rect& rect);
    Window(const Window &other);
    ~Window();

    Rect rect() const { return m_rect; }

    SharedPtr<PixelBuffer> buffer() { return m_buffer; }
    const SharedPtr<PixelBuffer> buffer() const { return m_buffer; }

private:
    String m_shm_path;
    Rect m_rect;
    SharedPtr<PixelBuffer> m_buffer;
};