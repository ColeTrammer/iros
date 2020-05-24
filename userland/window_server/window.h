#pragma once

#include <graphics/rect.h>
#include <liim/pointers.h>
#include <liim/string.h>

class PixelBuffer;

typedef uint64_t wid_t;

class Window {
public:
    Window(String shm_path, const Rect& rect, String title, int client_id);
    ~Window();

    Window(const Window& other) = delete;

    const Rect& rect() const { return m_rect; }
    const Rect& content_rect() const { return m_content_rect; }

    int close_button_x() const { return rect().x() + rect().width() - 13; }
    int close_button_y() const { return rect().y() + 10; }
    int close_button_radius() const { return 6; }

    wid_t id() const { return m_id; }
    int client_id() const { return m_client_id; }

    void swap() {
        auto temp = m_back_buffer;
        m_back_buffer = m_front_buffer;
        m_front_buffer = temp;
    }

    const String& title() const { return m_title; }

    SharedPtr<PixelBuffer>& buffer() { return m_front_buffer; }
    const SharedPtr<PixelBuffer>& buffer() const { return m_front_buffer; }

private:
    String m_shm_path;
    Rect m_rect;
    Rect m_content_rect;
    const wid_t m_id;
    String m_title;
    const int m_client_id;
    SharedPtr<PixelBuffer> m_front_buffer;
    SharedPtr<PixelBuffer> m_back_buffer;
};
