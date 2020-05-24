#include <graphics/pixel_buffer.h>
#include <graphics/renderer.h>
#include <sys/ioctl.h>

#include "window_manager.h"

constexpr int cursor_width = 12;
constexpr int cursor_height = 12;

// clang-format off
constexpr char cursor[cursor_height][cursor_width + 1] = { "............",
                                                           "...x........",
                                                           "...xx.......",
                                                           "...xxx......",
                                                           "...xxxx.....",
                                                           "...xxxxx....",
                                                           "...xxxxxx...",
                                                           "...xxxxx....",
                                                           "......xx....",
                                                           "......xx....",
                                                           ".......x....",
                                                           "............" };
// clang-format on

WindowManager::WindowManager(int fb, SharedPtr<PixelBuffer> front_buffer, SharedPtr<PixelBuffer> back_buffer)
    : m_fb(fb), m_front_buffer(front_buffer), m_back_buffer(back_buffer) {}

WindowManager::~WindowManager() {}

void WindowManager::add_window(SharedPtr<Window> window) {
    windows().add(window);
    set_active_window(window);
}

void WindowManager::remove_window(wid_t wid) {
    m_windows.remove_if([&](auto& window) {
        return window->id() == wid;
    });

    if (m_active_window->id() == wid) {
        if (windows().size() == 0) {
            set_active_window(nullptr);
        } else {
            set_active_window(windows().last());
        }
    }
}

void WindowManager::remove_windows_of_client(int client_id) {
    m_windows.for_each_reverse([&](auto& window) {
        if (window->client_id() == client_id) {
            remove_window(window->id());
        }
    });
}

void WindowManager::draw() {
    Renderer renderer(*m_back_buffer);
    renderer.set_color(Color(255, 255, 255));

    auto render_window = [&](auto& window) {
        renderer.draw_rect(window->rect());
        for (int x = window->rect().x(); x < window->rect().x() + window->rect().width(); x++) {
            m_back_buffer->put_pixel(x, window->rect().y() + 21, Color(255, 255, 255));
        }

        renderer.render_text(window->rect().x() + 5, window->rect().y() + 3, window->title());
        renderer.fill_circle(window->close_button_x(), window->close_button_y(), window->close_button_radius());

        auto& rect = window->content_rect();
        for (int x = rect.x(); x < rect.x() + rect.width(); x++) {
            for (int y = rect.y(); y < rect.y() + rect.height(); y++) {
                m_back_buffer->put_pixel(x, y, window->buffer()->get_pixel(x - rect.x(), y - rect.y()));
            }
        }
    };

    m_back_buffer->clear();

    for_each_window(render_window);

    for (int y = 0; y < cursor_height; y++) {
        for (int x = 0; x < cursor_width; x++) {
            if (cursor[y][x] == 'x') {
                renderer.pixels().put_pixel(m_mouse_x + x, m_mouse_y + y, Color(255, 255, 255));
            }
        }
    }

    swap_buffers();
}

void WindowManager::swap_buffers() {
    auto temp = m_back_buffer;
    m_back_buffer = m_front_buffer;
    m_front_buffer = temp;

    ioctl(m_fb, SSWAPBUF, m_front_buffer->pixels());
    memcpy(m_back_buffer->pixels(), m_front_buffer->pixels(), m_front_buffer->size_in_bytes());
}

void WindowManager::notify_mouse_moved(int dx, int dy, bool scale) {
    int computed_x = scale ? (m_mouse_x + dx * m_front_buffer->width() / 0xFFFF) : (m_mouse_x + dx);
    int computed_y = scale ? (m_mouse_y - dy * m_front_buffer->height() / 0xFFFF) : (m_mouse_y - dy);

    int new_mouse_x = LIIM::clamp(computed_x, 0, m_front_buffer->width() - cursor_width);
    int new_mouse_y = LIIM::clamp(computed_y, 0, m_front_buffer->height() - cursor_height);

    set_mouse_coordinates(new_mouse_x, new_mouse_y);
}

void WindowManager::set_active_window(SharedPtr<Window> window) {
    m_active_window = move(window);
}

int WindowManager::find_window_intersecting_point(Point p) {
    for (int i = windows().size() - 1; i >= 0; i--) {
        if (windows()[i]->rect().intersects(p)) {
            return i;
        }
    }

    return -1;
}

void WindowManager::notify_mouse_pressed() {
    int index = find_window_intersecting_point({ m_mouse_x, m_mouse_y });
    if (index == -1) {
        return;
    }

    auto& window = windows()[index];
    int dx = m_mouse_x - window->close_button_x();
    int dy = m_mouse_y - window->close_button_y();
    if (dx * dx + dy * dy <= window->close_button_radius() * window->close_button_radius()) {
        if (on_window_close_button_pressed) {
            on_window_close_button_pressed(*window);
        }
        return;
    }

    windows().rotate_left(index, windows().size());
    set_active_window(windows().last());
}

void WindowManager::set_mouse_coordinates(int x, int y) {
    if (m_mouse_x == x && m_mouse_y == y) {
        return;
    }

    m_mouse_x = x;
    m_mouse_y = y;
}
