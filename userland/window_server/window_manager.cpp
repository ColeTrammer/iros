#include <graphics/pixel_buffer.h>
#include <graphics/renderer.h>
#include <sys/ioctl.h>

#include "window_manager.h"

// #define WM_DRAW_DEBUG

constexpr int taskbar_height = 32;
constexpr int taskbar_item_x_spacing = 8;
constexpr int taskbar_item_y_spacing = 4;
constexpr int taskbar_item_width = 128;

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
    if (m_window_taskbar_items.empty()) {
        m_window_taskbar_items.add({ { taskbar_item_x_spacing, m_back_buffer->height() - taskbar_height + taskbar_item_y_spacing,
                                       taskbar_item_width, taskbar_height - 2 * taskbar_item_y_spacing },
                                     window });
    } else {
        auto& last_rect = m_window_taskbar_items.last();
        m_window_taskbar_items.add({ { last_rect.rect.x() + last_rect.rect.width() + taskbar_item_x_spacing,
                                       m_back_buffer->height() - taskbar_height + taskbar_item_y_spacing, taskbar_item_width,
                                       taskbar_height - 2 * taskbar_item_y_spacing },
                                     window });
    }
    set_active_window(window);
}

void WindowManager::remove_window(wid_t wid) {
    for (int i = 0; i < windows().size(); i++) {
        auto& window = *windows()[i];
        if (window.id() == wid) {
            windows().remove(i);
            m_dirty_rects.add(window.rect());
            for (i = 0; i < m_window_taskbar_items.size(); i++) {
                if (m_window_taskbar_items[i].window->id() != wid) {
                    continue;
                }

                m_window_taskbar_items.remove(i);
                for (; i < m_window_taskbar_items.size(); i++) {
                    auto& rect_to_move_left = m_window_taskbar_items[i].rect;
                    m_window_taskbar_items[i].rect.set_x(rect_to_move_left.x() - taskbar_item_x_spacing - taskbar_item_width);
                }
                break;
            }
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

    int taskbar_top = renderer.pixels().height() - taskbar_height;
    renderer.fill_rect(0, taskbar_top, renderer.pixels().width(), taskbar_height, ColorValue::Black);
    renderer.draw_line({ 0, taskbar_top }, { renderer.pixels().width() - 1, taskbar_top }, ColorValue::White);

    for (int i = 0; i < m_window_taskbar_items.size(); i++) {
        auto& item = m_window_taskbar_items[i];
        auto& rect = item.rect;
        auto& window = *item.window;
        renderer.draw_rect(rect, ColorValue::White);
        renderer.render_text(rect.x() + 4, rect.y() + 4, window.title(), ColorValue::White,
                             &window == m_active_window.get() ? Font::bold_font() : Font::default_font());
    }

    time_t now = time(nullptr);
    struct tm* tm = localtime(&now);
    auto time_string = String::format("%2d:%02d:%02d %s", tm->tm_hour % 12, tm->tm_min, tm->tm_sec, tm->tm_hour > 12 ? "PM" : "AM");

    renderer.render_text(m_back_buffer->width() - 100, taskbar_top + 4, time_string, ColorValue::White);
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

        auto& rect = window->content_rect();
        auto& src = *window->buffer();
        auto src_x_offset = rect.x();
        auto src_y_offset = rect.y();
        auto src_width = src.width();
        auto src_height = src.height();

        auto& dest = *m_back_buffer;
        auto dest_width = m_back_buffer->width();
        auto dest_height = m_back_buffer->height();

        auto src_x_start = 0;
        auto src_x_end = src_width;
        auto src_y_start = 0;
        auto src_y_end = src_height;

        if (src_x_offset + src_width <= 0 || src_x_offset >= dest_width || src_y_offset + src_height <= 0 || src_y_offset >= dest_height) {
            return;
        }

        if (src_x_offset < 0) {
            src_x_start += -src_x_offset;
            src_x_offset = 0;
        }
        if (src_x_offset + src_width > dest_width) {
            src_x_end = dest_width - src_x_offset;
        }

        if (src_y_offset < 0) {
            src_y_start += -src_y_offset;
            src_y_offset = 0;
        }
        if (src_y_offset + src_height > dest_height) {
            src_y_end = dest_height - src_y_offset;
        }

        auto* src_buffer = src.pixels();
        auto* dest_buffer = dest.pixels();
        auto src_row_size_in_bytes = (src_x_end - src_x_start) * sizeof(uint32_t);
        for (auto src_y = src_y_start; src_y < src_y_end; src_y++) {
            auto dest_y = src_y + src_y_offset;
            memcpy(dest_buffer + dest_y * dest_width + src_x_offset, src_buffer + src_y * src_width + src_x_start, src_row_size_in_bytes);
        }
    };

    for (auto& rect : m_dirty_rects) {
        renderer.fill_rect(rect, ColorValue::Black);
    }
    m_dirty_rects.clear();

    for_each_window(render_window);

    draw_taskbar(renderer);

    for (int y = 0; y < cursor_height; y++) {
        for (int x = 0; x < cursor_width; x++) {
            if (cursor[y][x] != '.') {
                auto c = cursor[y][x] == 'X' ? ColorValue::Black : ColorValue::White;
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

void WindowManager::notify_mouse_moved(int dx, int dy, bool absolue) {
    int computed_x = absolue ? (dx * m_front_buffer->width() / 0xFFFF) : (m_mouse_x + dx);
    int computed_y = absolue ? (dy * m_front_buffer->height() / 0xFFFF) : (m_mouse_y - dy);

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
    if (m_mouse_y >= m_back_buffer->height() - taskbar_height) {
        for (auto& item : m_window_taskbar_items) {
            if (item.rect.intersects({ m_mouse_x, m_mouse_y })) {
                for (int i = 0; i < windows().size(); i++) {
                    if (windows()[i] == item.window) {
                        windows().rotate_left(i, windows().size());
                        break;
                    }
                }
                set_active_window(item.window);
                break;
            }
        }
        return;
    }

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
