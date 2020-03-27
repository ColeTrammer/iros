#include <graphics/pixel_buffer.h>
#include <graphics/renderer.h>
#include <sys/ioctl.h>

#include "window_manager.h"

constexpr int cursor_width = 16;
constexpr int cursor_height = 16;

WindowManager::WindowManager(int fb, SharedPtr<PixelBuffer> front_buffer, SharedPtr<PixelBuffer> back_buffer)
    : m_fb(fb), m_front_buffer(front_buffer), m_back_buffer(back_buffer) {}

WindowManager::~WindowManager() {}

void WindowManager::add_window(SharedPtr<Window> window) {
    windows().add(window);
    m_active_window = window.get();
}

void WindowManager::draw() {
    Renderer renderer(*m_back_buffer);
    renderer.set_color(Color(255, 255, 255));

    auto render_window = [&](auto& window) {
        renderer.draw_rect(window->rect().x() - 1, window->rect().y() - 1, window->rect().width() + 2, window->rect().height() + 2);
        for (int x = window->rect().x(); x < window->rect().x() + window->rect().width(); x++) {
            for (int y = window->rect().y(); y < window->rect().y() + window->rect().height(); y++) {
                m_back_buffer->put_pixel(x, y, window->buffer()->get_pixel(x - window->rect().x(), y - window->rect().y()));
            }
        }
    };

    m_back_buffer->clear();

    for_each_window(render_window);

    renderer.fill_rect(m_mouse_x, m_mouse_y, cursor_width, cursor_height);

    swap_buffers();
}

void WindowManager::swap_buffers() {
    auto temp = m_back_buffer;
    m_back_buffer = m_front_buffer;
    m_front_buffer = temp;

    ioctl(m_fb, SSWAPBUF, m_front_buffer->pixels());
    memcpy(m_back_buffer->pixels(), m_front_buffer->pixels(), m_front_buffer->size_in_bytes());
}

void WindowManager::notify_mouse_moved(int dx, int dy) {
    int new_mouse_x = LIIM::clamp(m_mouse_x + dx, 0, m_front_buffer->width() * m_front_buffer->height() - cursor_width);
    int new_mouse_y = LIIM::clamp(m_mouse_y - dy, 0, m_front_buffer->width() * m_front_buffer->height() - cursor_height);

    set_mouse_coordinates(new_mouse_x, new_mouse_y);
}

void WindowManager::set_mouse_coordinates(int x, int y) {
    if (m_mouse_x == x && m_mouse_y == y) {
        return;
    }

    m_mouse_x = x;
    m_mouse_y = y;
}