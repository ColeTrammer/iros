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

    void swap_buffers(Connection& connection);

    wid_t wid() const { return m_wid; }

    template<typename C>
    void set_draw_callback(C c) {
        m_draw_callback = move(c);
    }

private:
    friend class Connection;

    static SharedPtr<Window> construct(const Rect& rect, Message::CreateWindowResponse& message);
    static void draw_all(Connection& connection);

    void draw(Connection& connection);

    Window(const Rect& rect, Message::CreateWindowResponse& message);

    Rect m_rect;
    SharedPtr<PixelBuffer> m_front;
    SharedPtr<PixelBuffer> m_back;
    wid_t m_wid;
    Function<void(SharedPtr<PixelBuffer>&)> m_draw_callback;
};

}