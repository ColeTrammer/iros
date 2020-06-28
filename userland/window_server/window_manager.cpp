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

static WindowManager* s_the;

WindowManager& WindowManager::the() {
    assert(s_the);
    return *s_the;
}

WindowManager::WindowManager(int fb, SharedPtr<PixelBuffer> front_buffer, SharedPtr<PixelBuffer> back_buffer)
    : m_fb(fb), m_front_buffer(front_buffer), m_back_buffer(back_buffer), m_taskbar(back_buffer->width(), back_buffer->height()) {
    s_the = this;
}

WindowManager::~WindowManager() {}

void WindowManager::add_window(SharedPtr<Window> window) {
    windows().add(window);
    m_taskbar.notify_window_added(window);
    set_active_window(window);
    invalidate_rect(window->rect());
}

void WindowManager::remove_window(wid_t wid) {
    for (int i = 0; i < windows().size(); i++) {
        auto& window = *windows()[i];
        if (window.id() == wid) {
            m_taskbar.notify_window_removed(window);
            invalidate_rect(window.rect());
            windows().remove(i);
            break;
        }
    }

    if (m_active_window->id() == wid) {
        if (windows().size() == 0) {
            set_active_window(nullptr);
        } else {
            set_active_window(windows().last());
        }
    }

    if (m_window_to_resize && m_window_to_resize->id() == wid) {
        m_window_to_resize = nullptr;
        m_window_resize_mode = ResizeMode::Invalid;
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

void WindowManager::draw() {
#ifdef WM_DRAW_DEBUG
    timespec start;
    clock_gettime(CLOCK_MONOTONIC, &start);
#endif /* WM_DRAW_DEBUG */

    Renderer renderer(*m_back_buffer);

    auto render_window = [&](auto& window) {
        renderer.fill_rect(window->rect().x() + 1, window->rect().y(), window->rect().width() - 1, 21, ColorValue::Black);
        renderer.draw_rect(window->rect(), ColorValue::White);
        renderer.draw_line({ window->rect().x(), window->rect().y() + 21 },
                           { window->rect().x() + window->rect().width() - 1, window->rect().y() + 21 }, ColorValue::White);
        renderer.render_text(window->rect().x() + 5, window->rect().y() + 3, window->title(), ColorValue::White);
        renderer.fill_circle(window->close_button_x(), window->close_button_y(), window->close_button_radius(), ColorValue::White);

        invalidate_rect({ window->rect().x(), window->rect().y(), window->rect().width(), 22 });
        invalidate_rect({ window->rect().x(), window->rect().y() + 22, 1, window->rect().height() });
        invalidate_rect({ window->rect().x(), window->rect().y() + window->rect().height() - 1, window->rect().width(), 1 });
        invalidate_rect({ window->rect().x() + window->rect().width() - 1, window->rect().y() + 22, 1, window->rect().height() });

        for (auto& r : m_dirty_rects) {
            auto dest_rect = window->content_rect().intersection_with(r);
            if (dest_rect == Rect()) {
                continue;
            }

            auto src_rect = dest_rect;
            src_rect.set_x(src_rect.x() - window->content_rect().x());
            src_rect.set_y(src_rect.y() - window->content_rect().y());
            renderer.draw_bitmap(*window->buffer(), src_rect, dest_rect);
        }
    };

    for (auto& rect : m_dirty_rects) {
        renderer.fill_rect(rect, ColorValue::Black);
    }

    for_each_window(render_window);

    m_taskbar.render(renderer);

    for (int y = 0; y < cursor_height; y++) {
        for (int x = 0; x < cursor_width; x++) {
            if (cursor[y][x] != '.') {
                auto c = cursor[y][x] == 'X' ? ColorValue::Black : ColorValue::White;
                renderer.pixels().put_pixel(m_mouse_x + x, m_mouse_y + y, c);
            }
        }
    }

    swap_buffers();
    m_dirty_rects.clear();

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

void WindowManager::notify_mouse_moved(int dx, int dy, bool absolue) {
    int computed_x = absolue ? (dx * m_front_buffer->width() / 0xFFFF) : (m_mouse_x + dx);
    int computed_y = absolue ? (dy * m_front_buffer->height() / 0xFFFF) : (m_mouse_y - dy);

    int new_mouse_x = LIIM::clamp(computed_x, 0, m_front_buffer->width() - cursor_width);
    int new_mouse_y = LIIM::clamp(computed_y, 0, m_front_buffer->height() - cursor_height);

    set_mouse_coordinates(new_mouse_x, new_mouse_y);
}

void WindowManager::set_active_window(SharedPtr<Window> window) {
    if (m_active_window == window) {
        return;
    }

    m_active_window = move(window);
    invalidate_rect(m_active_window->rect());
}

void WindowManager::move_to_front_and_make_active(SharedPtr<Window> window) {
    for (int i = 0; i < windows().size(); i++) {
        if (windows()[i] == window) {
            windows().rotate_left(i, windows().size());
            break;
        }
    }
    set_active_window(window);
}

int WindowManager::find_window_intersecting_point(Point p) {
    for (int i = windows().size() - 1; i >= 0; i--) {
        if (windows()[i]->rect().intersects(p)) {
            return i;
        }
    }

    return -1;
}

int WindowManager::find_window_intersecting_rect(const Rect& r) {
    for (int i = windows().size() - 1; i >= 0; i--) {
        if (windows()[i]->rect().intersects(r)) {
            return i;
        }
    }

    return -1;
}

void WindowManager::notify_mouse_pressed(mouse_button_state left, mouse_button_state right) {
    if (m_taskbar.notify_mouse_pressed(m_mouse_x, m_mouse_y, left, right)) {
        return;
    }

    auto cursor_rect = Rect(m_mouse_x, m_mouse_y, cursor_width, cursor_height);
    int index = find_window_intersecting_rect(cursor_rect);
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

    if (left == MOUSE_DOWN) {
        if (cursor_rect.intersects(window->rect().top_left())) {
            m_window_to_resize = window;
            m_window_resize_mode = ResizeMode::TopLeft;
        } else if (cursor_rect.intersects(window->rect().top_right())) {
            m_window_to_resize = window;
            m_window_resize_mode = ResizeMode::TopRight;
        } else if (cursor_rect.intersects(window->rect().bottom_right())) {
            m_window_to_resize = window;
            m_window_resize_mode = ResizeMode::BottomRight;
        } else if (cursor_rect.intersects(window->rect().bottom_left())) {
            m_window_to_resize = window;
            m_window_resize_mode = ResizeMode::BottomLeft;
        } else if (cursor_rect.intersects(window->rect().top_edge())) {
            m_window_to_resize = window;
            m_window_resize_mode = ResizeMode::Top;
        } else if (cursor_rect.intersects(window->rect().right_edge())) {
            m_window_to_resize = window;
            m_window_resize_mode = ResizeMode::Right;
        } else if (cursor_rect.intersects(window->rect().bottom_edge())) {
            m_window_to_resize = window;
            m_window_resize_mode = ResizeMode::Bottom;
        } else if (cursor_rect.intersects(window->rect().left_edge())) {
            m_window_to_resize = window;
            m_window_resize_mode = ResizeMode::Left;
        } else if (m_mouse_y <= window->rect().y() + 20) {
            // Mouse down on window title bar
            m_window_to_move = window;
            m_window_move_initial_location = window->rect().top_left();
            m_window_move_origin = { m_mouse_x, m_mouse_y };
        }
    }

    if (left == MOUSE_UP) {
        m_window_to_move = nullptr;
        m_window_to_resize = nullptr;
        m_window_resize_mode = ResizeMode::Invalid;
    }

    move_to_front_and_make_active(windows()[index]);
}

void WindowManager::set_mouse_coordinates(int x, int y) {
    if (m_mouse_x == x && m_mouse_y == y) {
        return;
    }

    invalidate_rect({ m_mouse_x, m_mouse_y, cursor_width, cursor_height });

    auto mouse_dx = x - m_mouse_x;
    auto mouse_dy = y - m_mouse_y;

    m_mouse_x = x;
    m_mouse_y = y;

    if (m_window_to_resize) {
        bool was_already_resizing = m_window_to_resize->in_resize();
        if (!was_already_resizing) {
            m_window_to_resize->resize_rect() = m_window_to_resize->rect();
        }

        switch (m_window_resize_mode) {
            case ResizeMode::TopLeft:
                m_window_to_resize->resize_rect().set_x(m_window_to_resize->resize_rect().x() + mouse_dx);
                m_window_to_resize->resize_rect().set_y(m_window_to_resize->resize_rect().y() + mouse_dy);
                mouse_dx *= -1;
                mouse_dy *= -1;
                break;
            case ResizeMode::Top:
                m_window_to_resize->resize_rect().set_y(m_window_to_resize->resize_rect().y() + mouse_dy);
                mouse_dx = 0;
                mouse_dy *= -1;
                break;
            case ResizeMode::TopRight:
                m_window_to_resize->resize_rect().set_y(m_window_to_resize->resize_rect().y() + mouse_dy);
                mouse_dy *= -1;
                break;
            case ResizeMode::Right:
                mouse_dy = 0;
                break;
            case ResizeMode::BottomRight:
                break;
            case ResizeMode::Bottom:
                mouse_dx = 0;
                break;
            case ResizeMode::BottomLeft:
                m_window_to_resize->resize_rect().set_x(m_window_to_resize->resize_rect().x() + mouse_dx);
                mouse_dx *= -1;
                break;
            case ResizeMode::Left:
                m_window_to_resize->resize_rect().set_x(m_window_to_resize->resize_rect().x() + mouse_dx);
                mouse_dx *= -1;
                mouse_dy = 0;
                break;
            default:
                assert(false);
        }

        m_window_to_resize->resize_rect().set_width(m_window_to_resize->resize_rect().width() + mouse_dx);
        m_window_to_resize->resize_rect().set_height(m_window_to_resize->resize_rect().height() + mouse_dy);

        if (!was_already_resizing) {
            m_window_to_resize->set_in_resize(true);
            if (on_window_resize_start) {
                on_window_resize_start(*m_window_to_resize);
            }
        }
    }

    if (m_window_to_move) {
        int dx = m_mouse_x - m_window_move_origin.x();
        int dy = m_mouse_y - m_window_move_origin.y();
        if (!dx && !dy) {
            return;
        }

        invalidate_rect(m_window_to_move->rect());
        m_window_to_move->set_x(m_window_move_initial_location.x() + dx);
        m_window_to_move->set_y(m_window_move_initial_location.y() + dy);
        invalidate_rect(m_window_to_move->rect());
    }
}
