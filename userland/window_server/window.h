#pragma once

#include <graphics/rect.h>
#include <liim/string.h>
#include <memory>

class PixelBuffer;

typedef uint64_t wid_t;

class Window {
public:
    Window(String shm_path, const Rect& rect, int client_id);
    ~Window();

    Window(const Window& other) = delete;

    Rect rect() const { return m_rect; }

    wid_t id() const { return m_id; }
    int client_id() const { return m_client_id; }

    void swap() {
        auto temp = m_back_buffer;
        m_back_buffer = m_front_buffer;
        m_front_buffer = temp;
    }

    std::shared_ptr<PixelBuffer> buffer() { return m_front_buffer; }
    const std::shared_ptr<PixelBuffer> buffer() const { return m_front_buffer; }

private:
    String m_shm_path;
    Rect m_rect;
    const wid_t m_id;
    const int m_client_id;
    std::shared_ptr<PixelBuffer> m_front_buffer;
    std::shared_ptr<PixelBuffer> m_back_buffer;
};