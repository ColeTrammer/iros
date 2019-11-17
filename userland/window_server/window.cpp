#include <graphics/pixel_buffer.h>

#include "window.h"

Window::Window(const Rect& rect) 
    : m_rect(rect)
    , m_buffer(new PixelBuffer(rect))
{
}

Window::~Window()
{
}