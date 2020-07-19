#include <fcntl.h>
#include <liim/hash_map.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <window_server/connection.h>
#include <window_server/message.h>
#include <window_server/window.h>

namespace WindowServer {

Window::Window(const Rect& rect, Message::CreateWindowResponse& created_data, Connection& connection)
    : m_rect(rect), m_wid(created_data.window_id), m_shm_path(created_data.shared_buffer_path), m_connection(connection) {
    resize(rect.width(), rect.height());
}

void Window::resize(int new_width, int new_height) {
    if (m_front) {
        munmap(m_front->pixels(), m_front->size_in_bytes());
    }
    if (m_back) {
        munmap(m_back->pixels(), m_back->size_in_bytes());
    }

    auto path = m_shm_path;
    int shm_front = shm_open(path.string(), O_RDWR, 0);
    path[path.size() - 1]++;
    int shm_back = shm_open(path.string(), O_RDWR, 0);
    assert(shm_front != -1);
    assert(shm_back != -1);

    void* front_raw_memory = mmap(nullptr, new_width * new_height * sizeof(uint32_t), PROT_WRITE | PROT_READ, MAP_SHARED, shm_front, 0);
    void* back_raw_memory = mmap(nullptr, new_width * new_height * sizeof(uint32_t), PROT_READ | PROT_WRITE, MAP_SHARED, shm_back, 0);
    assert(front_raw_memory != MAP_FAILED);
    assert(back_raw_memory != MAP_FAILED);

    close(shm_front);
    close(shm_back);

    m_front = PixelBuffer::wrap(reinterpret_cast<uint32_t*>(front_raw_memory), new_width, new_height);
    m_back = PixelBuffer::wrap(reinterpret_cast<uint32_t*>(back_raw_memory), new_width, new_height);

    m_rect.set_width(new_width);
    m_rect.set_height(new_height);
}

Window::~Window() {
    connection().windows().remove(wid());
}

void Window::swap_buffers() {
    connection().send_swap_buffer_request(wid());
    LIIM::swap(m_front, m_back);
    memcpy(m_back->pixels(), m_front->pixels(), m_front->size_in_bytes());
}

void Window::draw() {
    if (m_draw_callback) {
        m_draw_callback(m_back);
    }
    swap_buffers();
}

void Window::set_title(const String& title) {
    connection().send_window_rename_request(wid(), title);
}

SharedPtr<Window> Window::construct(const Rect& rect, Message::CreateWindowResponse& created_data, Connection& connection) {
    Window* window = new Window(rect, created_data, connection);
    connection.windows().put(window->wid(), window);
    return SharedPtr<Window>(window);
}

}
