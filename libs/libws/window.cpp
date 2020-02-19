#include <fcntl.h>
#include <liim/hash_map.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <window_server/connection.h>
#include <window_server/message.h>
#include <window_server/window.h>

namespace WindowServer {

static HashMap<wid_t, Window*>& windows() {
    static HashMap<wid_t, Window*> s_windows;
    return s_windows;
}

Window::Window(const Rect& rect, Message::CreateWindowResponse& created_data) : m_rect(rect), m_wid(created_data.window_id) {
    int shm_front = shm_open(created_data.shared_buffer_path, O_RDWR, 0);
    created_data.shared_buffer_path[strlen(created_data.shared_buffer_path) - 1]++;
    int shm_back = shm_open(created_data.shared_buffer_path, O_RDWR, 0);

    void* front_raw_memory = mmap(nullptr, created_data.shared_buffer_size, PROT_WRITE | PROT_READ, MAP_SHARED, shm_front, 0);
    void* back_raw_memory = mmap(nullptr, created_data.shared_buffer_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_back, 0);
    close(shm_front);
    close(shm_back);

    m_front = PixelBuffer::wrap(reinterpret_cast<uint32_t*>(front_raw_memory), rect.width(), rect.height());
    m_back = PixelBuffer::wrap(reinterpret_cast<uint32_t*>(back_raw_memory), rect.width(), rect.height());
}

Window::~Window() {
    windows().remove(wid());
}

void Window::swap_buffers(Connection& connection) {
    connection.send_swap_buffer_request(wid());
    LIIM::swap(m_front, m_back);
}

void Window::draw(Connection& connection) {
    m_back->clear();
    m_draw_callback(m_back);
    swap_buffers(connection);
}

SharedPtr<Window> Window::construct(const Rect& rect, Message::CreateWindowResponse& created_data) {
    Window* window = new Window(rect, created_data);
    windows().put(window->wid(), window);
    return SharedPtr<Window>(window);
}

void Window::draw_all(Connection& connection) {
    windows().for_each([&](auto window) {
        window->draw(connection);
    });
}

}