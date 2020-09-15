#include <assert.h>
#include <fcntl.h>
#include <graphics/bitmap.h>
#include <stdio.h>
#include <sys/mman.h>

#include "window.h"
#include "window_manager.h"

static wid_t get_next_id() {
    static wid_t next_wid = 1;
    return next_wid++;
}

SharedPtr<Window> Window::find_window_intersecting_point(SharedPtr<Window> window, const Point& p) {
    for (auto& child : window->children()) {
        auto result = Window::find_window_intersecting_point(child, p);
        if (result) {
            return result;
        }
    }

    return (window->visible() && window->rect().intersects(p)) ? window : nullptr;
}

SharedPtr<Window> Window::find_window_intersecting_rect(SharedPtr<Window> window, const Rect& r) {
    for (auto& child : window->children()) {
        auto result = Window::find_window_intersecting_rect(child, r);
        if (result) {
            return result;
        }
    }

    return (window->visible() && window->rect().intersects(r)) ? window : nullptr;
}

void Window::set_parent(SharedPtr<Window> child, SharedPtr<Window> parent) {
    child->m_parent = parent.get();
    parent->m_children.add(move(child));
}

Window::Window(const Rect& rect, String title, int client_id, WindowServer::WindowType type)
    : m_content_rect(rect), m_id(get_next_id()), m_title(title), m_client_id(client_id), m_type(type) {
    m_shm_path = String::format("/window_server_%lu", m_id);
    update_rect_from_content();
    map_buffers();
    m_front_buffer->clear();
    m_back_buffer->clear();
}

Window::~Window() {
    if (m_front_buffer && m_raw_buffer != MAP_FAILED) {
        munmap(m_raw_buffer, m_raw_buffer_size);
        shm_unlink(m_shm_path.string());
    }
}

void Window::update_rect_from_content() {
    switch (m_type) {
        case WindowServer::WindowType::Application:
            m_rect.set_x(m_content_rect.x() - 1);
            m_rect.set_y(m_content_rect.y() - 22);
            m_rect.set_width(m_content_rect.width() + 2);
            m_rect.set_height(m_content_rect.height() + 23);
            break;
        case WindowServer::WindowType::Frameless:
            m_rect = m_content_rect;
            break;
    }
}

void Window::update_content_from_rect() {
    switch (m_type) {
        case WindowServer::WindowType::Application:
            m_content_rect.set_x(m_rect.x() + 1);
            m_content_rect.set_y(m_rect.y() + 22);
            m_content_rect.set_width(m_rect.width() - 2);
            m_content_rect.set_height(m_rect.height() - 23);
            break;
        case WindowServer::WindowType::Frameless:
            m_content_rect = m_rect;
            break;
    }
}

void Window::set_position_relative_to_parent(int x, int y) {
    if (m_parent) {
        x += m_parent->content_rect().x();
        y += m_parent->content_rect().y();
    }
    set_position(x, y);
}

void Window::set_position(int x, int y) {
    int dx = x - m_rect.x();
    int dy = y - m_rect.y();

    m_rect.set_x(x);
    m_rect.set_y(y);
    update_content_from_rect();

    for (auto& child : m_children) {
        int child_x = child->rect().x() + dx;
        int child_y = child->rect().y() + dy;
        child->set_position(child_x, child_y);
    }
}

void Window::map_buffers() {
    int fd = shm_open(m_shm_path.string(), O_RDWR | O_CREAT, 0666);
    assert(fd != -1);

    size_t len = 2 * m_content_rect.width() * m_content_rect.height() * sizeof(uint32_t);
    ftruncate(fd, len);

    m_raw_buffer = mmap(nullptr, len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    assert(m_raw_buffer != MAP_FAILED);
    m_raw_buffer_size = len;

    m_front_buffer = Bitmap::wrap(reinterpret_cast<uint32_t*>(m_raw_buffer), m_content_rect.width(), m_content_rect.height());
    m_back_buffer = Bitmap::wrap(reinterpret_cast<uint32_t*>(m_raw_buffer) + m_front_buffer->size_in_bytes() / sizeof(uint32_t),
                                 m_content_rect.width(), m_content_rect.height());
    close(fd);
}

void Window::relative_resize(int delta_x, int delta_y) {
    if (delta_x == 0 && delta_y == 0) {
        return;
    }

    auto old_width = m_content_rect.width();
    auto old_height = m_content_rect.height();

    WindowManager::the().invalidate_rect(m_rect);

    m_rect.set_width(m_rect.width() + delta_x);
    m_rect.set_height(m_rect.height() + delta_y);
    update_content_from_rect();

    WindowManager::the().invalidate_rect(m_rect);

    // Make sure the 'front_buffer' is the first buffer in the file, so that it will be synced up with the
    // client when they re-map in the buffer. For the purpose of resizing, the back buffer can be safely
    // ignored (the garbage in it will be overwritten by the application), but the front buffer needs to be
    // properly adjusted so that any repaints of the window before the application reacts to the resizing will
    // look alright.
    if (m_front_buffer && m_back_buffer && m_front_buffer->pixels() > m_back_buffer->pixels()) {
        LIIM::swap(m_front_buffer, m_back_buffer);
    }

    if (m_front_buffer && delta_x < 0) {
        m_front_buffer->shrink_width(m_content_rect.width());
    }

    munmap(m_raw_buffer, m_raw_buffer_size);
    m_raw_buffer = MAP_FAILED;

    map_buffers();
    if (delta_x > 0) {
        m_front_buffer->adjust_for_size_change(old_width, old_height);
    } else if (delta_y > 0) {
        m_front_buffer->clear_after_y(old_height);
    }
}

void Window::did_remove() {
    if (m_parent) {
        m_parent->children().remove_if([this](auto& window) {
            return window.get() == this;
        });
    }

    m_children.for_each_reverse([this](auto& child) {
        WindowManager::the().remove_window(child);
    });
}

void Window::swap() {
    LIIM::swap(m_front_buffer, m_back_buffer);
    WindowManager::the().invalidate_rect(m_content_rect);
}
