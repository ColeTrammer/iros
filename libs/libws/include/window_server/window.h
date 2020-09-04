#pragma once

#include <graphics/pixel_buffer.h>
#include <graphics/rect.h>
#include <liim/function.h>
#include <liim/pointers.h>
#include <sys/mman.h>
#include <window_server/message.h>

namespace WindowServer {

class Connection;

class Window {
public:
    ~Window();

    void swap_buffers();
    void set_visibility(int x, int y, bool visible);

    wid_t wid() const { return m_wid; }

    template<typename C>
    void set_draw_callback(C c) {
        m_draw_callback = move(c);
    }

    void draw();

    SharedPtr<PixelBuffer>& pixels() { return m_back; }

    const Rect& rect() const { return m_rect; }

    void resize(int new_width, int new_height);

    void set_title(const String& title);

    bool removed() const { return m_removed; }
    void set_removed(bool b) { m_removed = b; }
    void remove();

private:
    friend class Connection;

    static SharedPtr<Window> construct(const Rect& rect, Message::CreateWindowResponse& message, Connection& connection);

    Connection& connection() { return m_connection; }

    Window(const Rect& rect, Message::CreateWindowResponse& message, Connection& connection);

    Rect m_rect;
    SharedPtr<PixelBuffer> m_front;
    SharedPtr<PixelBuffer> m_back;
    void* m_raw_pixels { MAP_FAILED };
    size_t m_raw_pixels_size { 0 };
    wid_t m_wid;
    String m_shm_path;
    Function<void(SharedPtr<PixelBuffer>&)> m_draw_callback;
    Connection& m_connection;
    bool m_removed { false };
};

}
