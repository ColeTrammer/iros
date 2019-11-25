#include <graphics/pixel_buffer.h>

#include "window_manager.h"

WindowManager::WindowManager(std::shared_ptr<PixelBuffer> pixels)
    : m_pixels(pixels)
{
}

WindowManager::~WindowManager()
{
}

void WindowManager::add_window(std::shared_ptr<Window> window)
{
    windows().add(window);
}

void WindowManager::draw()
{
    m_pixels->clear();
    auto render_window = [&](auto& window) {
        for (int x = window->rect().x(); x < window->rect().x() + window->rect().width(); x++) {
            for (int y = window->rect().y(); y < window->rect().y() + window->rect().height(); y++) {
                m_pixels->put_pixel(x, y, window->buffer()->get_pixel(x - window->rect().x(), y - window->rect().y()));
            }
        }
    };

    for_each_window(render_window);
}