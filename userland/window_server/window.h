#pragma once

#include <liim/string.h>
#include <graphics/rect.h>
#include <memory>

class PixelBuffer;

class Window {
public:
    Window(const String& shm_path, const Rect& rect);
    Window(const Window &other);
    ~Window();

    static std::shared_ptr<Window> from_shm_and_rect(const String& shm_path, const Rect& rect)
    {
        return std::make_shared<Window>(shm_path, rect);
    }

    Rect rect() const { return m_rect; }

    std::shared_ptr<PixelBuffer> buffer() { return m_buffer; }
    const std::shared_ptr<PixelBuffer> buffer() const { return m_buffer; }

private:
    String m_shm_path;
    Rect m_rect;
    std::shared_ptr<PixelBuffer> m_buffer;
};