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
    if (m_raw_pixels != MAP_FAILED) {
        munmap(m_raw_pixels, m_raw_pixels_size);
        m_raw_pixels = MAP_FAILED;
    }

    int shm = shm_open(m_shm_path.string(), O_RDWR, 0);
    assert(shm != -1);

    m_raw_pixels_size = 2 * new_width * new_height * sizeof(uint32_t);
    m_raw_pixels = mmap(nullptr, m_raw_pixels_size, PROT_WRITE | PROT_READ, MAP_SHARED, shm, 0);
    assert(m_raw_pixels != MAP_FAILED);
    close(shm);

    m_front = Bitmap::wrap(reinterpret_cast<uint32_t*>(m_raw_pixels), new_width, new_height, false);
    m_back =
        Bitmap::wrap(reinterpret_cast<uint32_t*>(m_raw_pixels) + m_raw_pixels_size / 2 / sizeof(uint32_t), new_width, new_height, false);

    m_rect.set_width(new_width);
    m_rect.set_height(new_height);
}

void Window::remove() {
    connection().send_remove_window_request(wid());
    m_removed = true;
}

Window::~Window() {
    if (!removed()) {
        remove();
    }
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

void Window::set_visibility(int x, int y, bool visible) {
    connection().send_change_window_visibility_request(wid(), x, y, visible);
}

SharedPtr<Window> Window::construct(const Rect& rect, Message::CreateWindowResponse& created_data, Connection& connection) {
    Window* window = new Window(rect, created_data, connection);
    return SharedPtr<Window>(window);
}

}
