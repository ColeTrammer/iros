#include <graphics/pixel_buffer.h>
#include <sys/ioctl.h>

#include "window_manager.h"

WindowManager::WindowManager(int fb, SharedPtr<PixelBuffer> front_buffer, SharedPtr<PixelBuffer> back_buffer)
    : m_fb(fb), m_front_buffer(front_buffer), m_back_buffer(back_buffer) {}

WindowManager::~WindowManager() {}

void WindowManager::add_window(SharedPtr<Window> window) {
    windows().add(window);
}

void WindowManager::draw() {
    m_back_buffer->clear();
    auto render_window = [&](auto& window) {
        for (int x = window->rect().x(); x < window->rect().x() + window->rect().width(); x++) {
            for (int y = window->rect().y(); y < window->rect().y() + window->rect().height(); y++) {
                m_back_buffer->put_pixel(x, y, window->buffer()->get_pixel(x - window->rect().x(), y - window->rect().y()));
            }
        }
    };

    for_each_window(render_window);
    swap_buffers();
}

void WindowManager::swap_buffers() {
    auto temp = m_back_buffer;
    m_back_buffer = m_front_buffer;
    m_front_buffer = temp;

    ioctl(m_fb, SSWAPBUF, m_front_buffer->pixels());
}