#include <graphics/bitmap.h>
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

WindowManager::WindowManager(int fb, SharedPtr<Bitmap> front_buffer, SharedPtr<Bitmap> back_buffer)
    : m_fb(fb), m_front_buffer(front_buffer), m_back_buffer(back_buffer), m_taskbar(back_buffer->width(), back_buffer->height()) {
    s_the = this;

    int shared_palette_fd = shm_open("/.shared_theme", O_RDWR | O_CREAT, 0644);
    assert(shared_palette_fd != -1);
    assert(ftruncate(shared_palette_fd, Palette::byte_size()) == 0);
    close(shared_palette_fd);

    m_palette = Palette::create_from_shared_memory("/.shared_theme", PROT_READ | PROT_WRITE);
    auto palette = Palette::create_from_json("/usr/share/themes/default.json");
    m_palette->copy_from(*palette);
}

WindowManager::~WindowManager() {}

void WindowManager::add_window(SharedPtr<Window> window) {
    m_window_map.put(window->id(), window);

    if (window->visible()) {
        if (!window->parent()) {
            m_window_stack.add(window);
            set_active_window(window);
        } else {
            move_to_front_and_make_active(window);
        }
        invalidate_rect(window->rect());
    }

    m_taskbar.notify_window_added(window);
}

void WindowManager::set_window_visibility(SharedPtr<Window> window, bool visible) {
    if (window->visible() == visible) {
        return;
    }

    window->set_visible(visible);
    invalidate_rect(window->rect());
    if (visible) {
        if (!window->parent()) {
            m_window_stack.add(window);
            set_active_window(window);
        } else {
            move_to_front_and_make_active(window);
        }
    } else {
        if (!window->parent()) {
            m_window_stack.remove_element(window);
        }
        cleanup_active_window_state(window);
    }
    m_taskbar.notify_window_visibility_changed(move(window));
}

void WindowManager::cleanup_active_window_state(SharedPtr<Window> window) {
    if (m_active_window == window) {
        auto* parent = window->parent();
        if (parent && parent->visible()) {
            set_active_window(find_by_wid(parent->id()));
        } else {
            set_active_window(nullptr);
        }
    }

    if (m_window_to_resize == window) {
        m_window_to_resize = nullptr;
        m_window_resize_mode = ResizeMode::Invalid;
    }

    if (m_window_to_move == window) {
        m_window_to_move = nullptr;
    }
}

void WindowManager::remove_window(SharedPtr<Window> window) {
    m_window_map.remove(window->id());
    m_window_stack.remove_element(window);

    m_taskbar.notify_window_removed(*window);
    invalidate_rect(window->rect());

    cleanup_active_window_state(window);
    window->did_remove();
}

Point WindowManager::mouse_position_relative_to_window(const Window& window) const {
    return { m_mouse_x - window.content_rect().x(), m_mouse_y - window.content_rect().y() };
}

void WindowManager::remove_windows_of_client(SharedPtr<IPC::Endpoint> client) {
    Vector<SharedPtr<Window>> windows_to_remove;
    m_window_map.for_each([&](auto& window) {
        if (&window->client() == client.get()) {
            windows_to_remove.add(window);
        }
    });
    for (auto& window : windows_to_remove) {
        remove_window(window);
    }
}

void WindowManager::draw() {
    m_drawing = true;

#ifdef WM_DRAW_DEBUG
    timespec start;
    clock_gettime(CLOCK_MONOTONIC, &start);
#endif /* WM_DRAW_DEBUG */

    Renderer renderer(*m_back_buffer);

    Function<void(SharedPtr<Window>&)> render_window = [&](auto& window) {
        if (!window->visible()) {
            return;
        }

        if (window->type() == WindowServer::WindowType::Application) {
            auto title_bar_rect = Rect { window->rect().x() + 1, window->rect().y() + 1, window->rect().width() - 1, 20 };
            renderer.fill_rect(title_bar_rect, palette()->color(Palette::Background));
            renderer.render_text(window->title(), title_bar_rect.adjusted(-4, 0), palette()->color(Palette::Text));

            renderer.draw_rect(window->rect(), palette()->color(Palette::Outline));
            renderer.draw_line({ window->rect().x(), window->rect().y() + 21 },
                               { window->rect().x() + window->rect().width() - 1, window->rect().y() + 21 },
                               palette()->color(Palette::Outline));
            renderer.fill_circle(window->close_button_x(), window->close_button_y(), window->close_button_radius(),
                                 palette()->color(Palette::Text));

            invalidate_rect({ window->rect().x(), window->rect().y(), window->rect().width(), 22 });
            invalidate_rect({ window->rect().x(), window->rect().y() + 22, 1, window->rect().height() });
            invalidate_rect({ window->rect().x(), window->rect().y() + window->rect().height() - 1, window->rect().width(), 1 });
            invalidate_rect({ window->rect().x() + window->rect().width() - 1, window->rect().y() + 22, 1, window->rect().height() });
        }

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

        for (auto& child : window->children()) {
            render_window(child);
        }
    };

    for (auto& rect : m_dirty_rects) {
        renderer.fill_rect(rect, ColorValue::DarkGray);
    }

    for (auto& window : m_window_stack) {
        render_window(window);
    }

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

    m_drawing = false;
}

void WindowManager::swap_buffers() {
    auto temp = m_back_buffer;
    m_back_buffer = m_front_buffer;
    m_front_buffer = temp;

    ioctl(m_fb, SSWAPBUF, m_front_buffer->pixels());
    memcpy(m_back_buffer->pixels(), m_front_buffer->pixels(), m_front_buffer->size_in_bytes());
}

SharedPtr<Window> WindowManager::find_by_wid(wid_t id) {
    auto* ret = m_window_map.get(id);
    return ret ? *ret : nullptr;
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

    if (m_active_window) {
        if (on_window_state_change) {
            on_window_state_change(m_active_window, false);
        }
    }

    m_active_window = move(window);
    if (m_active_window) {
        if (on_window_state_change) {
            on_window_state_change(m_active_window, true);
        }
        invalidate_rect(m_active_window->rect());
    }
}

void WindowManager::move_to_front_and_make_active(SharedPtr<Window> window) {
    auto* root_window = window->root();
    for (int i = 0; i < m_window_stack.size(); i++) {
        if (m_window_stack[i].get() == root_window) {
            m_window_stack.rotate_left(i, m_window_stack.size());
            break;
        }
    }
    set_active_window(window);
}

SharedPtr<Window> WindowManager::find_window_intersecting_point(Point p) {
    for (int i = m_window_stack.size() - 1; i >= 0; i--) {
        auto result = Window::find_window_intersecting_point(m_window_stack[i], p);
        if (result) {
            return result;
        }
    }

    return nullptr;
}

SharedPtr<Window> WindowManager::find_window_intersecting_rect(const Rect& r) {
    for (int i = m_window_stack.size() - 1; i >= 0; i--) {
        auto result = Window::find_window_intersecting_rect(m_window_stack[i], r);
        if (result) {
            return result;
        }
    }

    return nullptr;
}

void WindowManager::notify_mouse_pressed(mouse_button_state left, mouse_button_state right) {
    if (m_taskbar.notify_mouse_pressed(m_mouse_x, m_mouse_y, left, right)) {
        return;
    }

    if (left == MOUSE_UP) {
        m_window_to_move = nullptr;
        m_window_to_resize = nullptr;
        m_window_resize_mode = ResizeMode::Invalid;
        return;
    }

    auto cursor_rect = Rect(m_mouse_x, m_mouse_y, cursor_width, cursor_height);
    auto window = find_window_intersecting_rect(cursor_rect);
    if (!window) {
        return;
    }

    int dx = m_mouse_x - window->close_button_x();
    int dy = m_mouse_y - window->close_button_y();
    if (window->type() == WindowServer::WindowType::Application &&
        dx * dx + dy * dy <= window->close_button_radius() * window->close_button_radius()) {
        if (on_window_close_button_pressed) {
            on_window_close_button_pressed(window);
        }
        return;
    }

    if (left == MOUSE_DOWN) {
        if (window->resizable() && cursor_rect.intersects(window->rect().top_left())) {
            m_window_to_resize = window;
            m_window_resize_mode = ResizeMode::TopLeft;
        } else if (window->resizable() && cursor_rect.intersects(window->rect().top_right())) {
            m_window_to_resize = window;
            m_window_resize_mode = ResizeMode::TopRight;
        } else if (window->resizable() && cursor_rect.intersects(window->rect().bottom_right())) {
            m_window_to_resize = window;
            m_window_resize_mode = ResizeMode::BottomRight;
        } else if (window->resizable() && cursor_rect.intersects(window->rect().bottom_left())) {
            m_window_to_resize = window;
            m_window_resize_mode = ResizeMode::BottomLeft;
        } else if (window->resizable() && cursor_rect.intersects(window->rect().top_edge())) {
            m_window_to_resize = window;
            m_window_resize_mode = ResizeMode::Top;
        } else if (window->resizable() && cursor_rect.intersects(window->rect().right_edge())) {
            m_window_to_resize = window;
            m_window_resize_mode = ResizeMode::Right;
        } else if (window->resizable() && cursor_rect.intersects(window->rect().bottom_edge())) {
            m_window_to_resize = window;
            m_window_resize_mode = ResizeMode::Bottom;
        } else if (window->resizable() && cursor_rect.intersects(window->rect().left_edge())) {
            m_window_to_resize = window;
            m_window_resize_mode = ResizeMode::Left;
        } else if (window->movable() && m_mouse_y <= window->rect().y() + 20) {
            // Mouse down on window title bar
            m_window_to_move = window;
            m_window_move_initial_location = window->rect().top_left();
            m_window_move_origin = { m_mouse_x, m_mouse_y };
        }
    }

    if (left == MOUSE_DOWN || right == MOUSE_DOWN) {
        move_to_front_and_make_active(window);
    }
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
                mouse_dx *= -1;
                mouse_dy *= -1;
                if (m_window_to_resize->resize_rect().width() + mouse_dx > 200) {
                    m_window_to_resize->resize_rect().set_x(m_window_to_resize->resize_rect().x() - mouse_dx);
                }
                if (m_window_to_resize->resize_rect().height() + mouse_dy > 200) {
                    m_window_to_resize->resize_rect().set_y(m_window_to_resize->resize_rect().y() - mouse_dy);
                }
                break;
            case ResizeMode::Top:
                mouse_dx = 0;
                mouse_dy *= -1;
                if (m_window_to_resize->resize_rect().height() + mouse_dy > 200) {
                    m_window_to_resize->resize_rect().set_y(m_window_to_resize->resize_rect().y() - mouse_dy);
                }
                break;
            case ResizeMode::TopRight:
                mouse_dy *= -1;
                if (m_window_to_resize->resize_rect().height() + mouse_dy > 200) {
                    m_window_to_resize->resize_rect().set_y(m_window_to_resize->resize_rect().y() - mouse_dy);
                }
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
                mouse_dx *= -1;
                if (m_window_to_resize->resize_rect().width() + mouse_dx > 200) {
                    m_window_to_resize->resize_rect().set_x(m_window_to_resize->resize_rect().x() - mouse_dx);
                }
                break;
            case ResizeMode::Left:
                mouse_dx *= -1;
                mouse_dy = 0;
                if (m_window_to_resize->resize_rect().width() + mouse_dx > 200) {
                    m_window_to_resize->resize_rect().set_x(m_window_to_resize->resize_rect().x() - mouse_dx);
                }
                break;
            default:
                assert(false);
        }

        m_window_to_resize->resize_rect().set_width(max(200, m_window_to_resize->resize_rect().width() + mouse_dx));
        m_window_to_resize->resize_rect().set_height(max(200, m_window_to_resize->resize_rect().height() + mouse_dy));

        if (!was_already_resizing) {
            m_window_to_resize->set_in_resize(true);
            if (on_window_resize_start) {
                on_window_resize_start(m_window_to_resize);
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
        m_window_to_move->set_position(m_window_move_initial_location.x() + dx, m_window_move_initial_location.y() + dy);
        invalidate_rect(m_window_to_move->rect());
    }
}

void WindowManager::invalidate_rect(const Rect& rect) {
    m_dirty_rects.add(rect);
    if (on_rect_invaliadted) {
        on_rect_invaliadted();
    }
}
