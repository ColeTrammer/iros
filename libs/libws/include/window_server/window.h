#pragma once

#include <graphics/pixel_buffer.h>
#include <graphics/rect.h>
#include <liim/function.h>
#include <liim/pointers.h>
#include <window_server/message.h>

namespace WindowServer {

class Connection;

class Window {
public:
    ~Window();

    void swap_buffers();

    wid_t wid() const { return m_wid; }

    template<typename C>
    void set_draw_callback(C c) {
        m_draw_callback = move(c);
    }

    void draw();

    SharedPtr<PixelBuffer>& pixels() { return m_back; }

private:
    friend class Connection;

    static SharedPtr<Window> construct(const Rect& rect, Message::CreateWindowResponse& message, Connection& connection);

    Connection& connection() { return m_connection; }

    Window(const Rect& rect, Message::CreateWindowResponse& message, Connection& connection);

    Rect m_rect;
    SharedPtr<PixelBuffer> m_front;
    SharedPtr<PixelBuffer> m_back;
    wid_t m_wid;
    Function<void(SharedPtr<PixelBuffer>&)> m_draw_callback;
    Connection& m_connection;
};

}