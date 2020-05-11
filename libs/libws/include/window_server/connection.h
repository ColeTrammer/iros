#pragma once

#include <liim/function.h>
#include <liim/hash_map.h>
#include <liim/pointers.h>
#include <window_server/message.h>
#include <window_server/window.h>

class PixelBuffer;

namespace WindowServer {

class Window;

class Connection {
public:
    Connection();
    ~Connection();

    SharedPtr<Window> create_window(int x, int y, int width, int height, const String& name);

    void send_swap_buffer_request(wid_t wid);

    template<typename C>
    void set_draw_callback(SharedPtr<Window>& window, C callback) {
        window->set_draw_callback(move(callback));
        setup_timer();
    }

    int fd() const { return m_fd; }

private:
    friend class Window;

    auto& windows() { return m_windows; }

    void setup_timer();

    HashMap<wid_t, Window*> m_windows;
    int m_fd;
};

}
