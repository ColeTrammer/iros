#pragma once

#include <liim/function.h>
#include <liim/hash_map.h>
#include <liim/pointers.h>
#include <window_server/message.h>
#include <window_server/window.h>

class Bitmap;

namespace WindowServer {

class Window;

class Connection {
public:
    Connection();
    ~Connection();

    SharedPtr<Window> create_window(int x, int y, int width, int height, const String& name, bool has_alpha, WindowType type,
                                    wid_t parent_id);

    void send_swap_buffer_request(wid_t wid);
    UniquePtr<Message> send_window_ready_to_resize_message(wid_t wid);
    void send_window_rename_request(wid_t wid, const String& name);
    void send_change_window_visibility_request(wid_t wid, int x, int y, bool visible);
    void send_remove_window_request(wid_t wid);

    template<typename C>
    void set_draw_callback(SharedPtr<Window>& window, C callback) {
        window->set_draw_callback(move(callback));
    }

    int fd() const { return m_fd; }

    UniquePtr<Message> recieve_message();
    void read_from_server();

private:
    Vector<UniquePtr<Message>> m_messages;
    int m_fd;
};

}
