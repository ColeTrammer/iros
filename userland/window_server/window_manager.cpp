#include <graphics/pixel_buffer.h>
#include <graphics/renderer.h>
#include <sys/ioctl.h>

#include "window_manager.h"

// #define WM_DRAW_DEBUG

constexpr int cursor_width = 12;
constexpr int cursor_height = 12;

// clang-format off
constexpr char cursor[cursor_height][cursor_width + 1] = { "..XX........",
                                                           "..XxX.......",
                                                           "..XxxX......",
                                                           "..XxxxX.....",
                                                           "..XxxxxX....",
                                                           "..XxxxxxX...",
                                                           "..XxxxxxxX..",
                                                           "..XxxxxxXX..",
                                                           "..XXXXxxX...",
                                                           ".....XxxX...",
                                                           "......XxX...",
                                                           ".......XX..." };
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
        if (window->id() == wid) {
            m_dirty_rects.add(window->rect());
            return true;
        }
        return false;
    });

    if (m_active_window->id() == wid) {
        if (windows().size() == 0) {
            set_active_window(nullptr);
        } else {
            set_active_window(windows().last());
        }
    }

    if (m_window_to_move && m_window_to_move->id() == wid) {
        m_window_to_move = nullptr;
    }
}

Point WindowManager::mouse_position_relative_to_window(const Window& window) const {
    return { m_mouse_x - window.content_rect().x(), m_mouse_y - window.content_rect().y() };
}

void WindowManager::remove_windows_of_client(int client_id) {
    m_windows.for_each_reverse([&](auto& window) {
        if (window->client_id() == client_id) {
            remove_window(window->id());
        }
    });
}

void WindowManager::draw_taskbar(Renderer& renderer) {
    constexpr int taskbar_height = 32;

    int taskbar_top = renderer.pixels().height() - taskbar_height;
    renderer.set_color(Color(0, 0, 0));
    renderer.fill_rect(0, taskbar_top, renderer.pixels().width(), taskbar_height);

    for (int x = 0; x < renderer.pixels().width(); x++) {
        renderer.pixels().put_pixel(x, taskbar_top, Color(255, 255, 255));
    }

    time_t now = time(nullptr);
    struct tm* tm = localtime(&now);
    auto time_string = String::format("%2d:%02d:%02d %s", tm->tm_hour % 12, tm->tm_min, tm->tm_sec, tm->tm_hour > 12 ? "PM" : "AM");

    renderer.set_color(Color(255, 255, 255));
    renderer.render_text(2, taskbar_top + 2, time_string);
}

void WindowManager::draw() {
#ifdef WM_DRAW_DEBUG
    timespec start;
    clock_gettime(CLOCK_MONOTONIC, &start);
#endif /* WM_DRAW_DEBUG */

    Renderer renderer(*m_back_buffer);

    auto render_window = [&](auto& window) {
        renderer.set_color(Color(0, 0, 0));
        renderer.fill_rect(window->rect().x() + 1, window->rect().y(), window->rect().width() - 1, 21);
        renderer.set_color(Color(255, 255, 255));

        renderer.draw_rect(window->rect());
        for (int x = window->rect().x(); x < window->rect().x() + window->rect().width(); x++) {
            m_back_buffer->put_pixel(x, window->rect().y() + 21, Color(255, 255, 255));
        }

        renderer.render_text(window->rect().x() + 5, window->rect().y() + 3, window->title());
        renderer.fill_circle(window->close_button_x(), window->close_button_y(), window->close_button_radius());

        auto& rect = window->content_rect();
        for (int x = LIIM::max(0, rect.x()); x < m_back_buffer->width() && x < rect.x() + rect.width(); x++) {
            for (int y = LIIM::max(rect.y(), 0); y < m_back_buffer->height() && y < rect.y() + rect.height(); y++) {
                m_back_buffer->put_pixel(x, y, window->buffer()->get_pixel(x - rect.x(), y - rect.y()));
            }
        }
    };

    renderer.set_color(Color(0, 0, 0));
    for (auto& rect : m_dirty_rects) {
        renderer.fill_rect(rect);
    }
    m_dirty_rects.clear();
    renderer.set_color(Color(255, 255, 255));

    for_each_window(render_window);

    draw_taskbar(renderer);

    renderer.set_color(Color(255, 255, 255));
    for (int y = 0; y < cursor_height; y++) {
        for (int x = 0; x < cursor_width; x++) {
            if (cursor[y][x] != '.') {
                auto c = cursor[y][x] == 'X' ? Color(0, 0, 0) : Color(255, 255, 255);
                renderer.pixels().put_pixel(m_mouse_x + x, m_mouse_y + y, c);
            }
        }
    }

    swap_buffers();

#ifdef WM_DRAW_DEBUG
    timespec end;
    clock_gettime(CLOCK_MONOTONIC, &end);

    long delta_seconds = end.tv_sec - start.tv_sec;
    long delta_nano_seconds = end.tv_nsec - start.tv_nsec;
    time_t delta_milli_seconds = delta_seconds * 1000 + delta_nano_seconds / 1000000;
    fprintf(stderr, "WindowManager::draw() took %lu ms\n", delta_milli_seconds);
#endif /* WM_DRAW_DEBUG */
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

void WindowManager::notify_mouse_pressed(mouse_button_state left, mouse_button_state) {
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

    if (left == MOUSE_DOWN && m_mouse_y <= window->rect().y() + 20) {
        // Mouse down on window title bar
        m_window_to_move = window;
        m_window_move_initial_location = window->rect().top_left();
        m_window_move_origin = { m_mouse_x, m_mouse_y };
    }

    if (left == MOUSE_UP) {
        m_window_to_move = nullptr;
    }

    windows().rotate_left(index, windows().size());
    set_active_window(windows().last());
}

void WindowManager::set_mouse_coordinates(int x, int y) {
    if (m_mouse_x == x && m_mouse_y == y) {
        return;
    }

    m_dirty_rects.add({ m_mouse_x, m_mouse_y, cursor_width, cursor_height });

    m_mouse_x = x;
    m_mouse_y = y;

    if (m_window_to_move) {
        int dx = m_mouse_x - m_window_move_origin.x();
        int dy = m_mouse_y - m_window_move_origin.y();
        if (!dx && !dy) {
            return;
        }

        m_dirty_rects.add(m_window_to_move->rect());
        m_window_to_move->set_x(m_window_move_initial_location.x() + dx);
        m_window_to_move->set_y(m_window_move_initial_location.y() + dy);
        m_dirty_rects.add(m_window_to_move->rect());
    }
}
