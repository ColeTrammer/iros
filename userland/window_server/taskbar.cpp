#include <graphics/renderer.h>

#include "taskbar.h"
#include "window_manager.h"

constexpr int taskbar_height = 32;
constexpr int taskbar_item_x_spacing = 8;
constexpr int taskbar_item_y_spacing = 4;
constexpr int taskbar_item_width = 128;

Taskbar::Taskbar(int display_width, int display_height) : m_display_width(display_width), m_display_height(display_height) {
    m_time_timer = App::Timer::create_interval_timer(
        nullptr,
        [this](int) {
            auto screen_rect = WindowManager::the().screen_rect();
            WindowManager::the().invalidate_rect({ 0, screen_rect.height() - taskbar_height, screen_rect.width(), taskbar_height });
        },
        1000);
}

void Taskbar::add_item(SharedPtr<Window> window) {
    if (window->type() != WindowServer::WindowType::Application || window->parent()) {
        return;
    }

    if (m_items.empty()) {
        m_items.add({ { taskbar_item_x_spacing, m_display_height - taskbar_height + taskbar_item_y_spacing, taskbar_item_width,
                        taskbar_height - 2 * taskbar_item_y_spacing },
                      window });
    } else {
        auto& last_rect = m_items.last().rect;
        m_items.add({ { last_rect.x() + last_rect.width() + taskbar_item_x_spacing, last_rect.y(), last_rect.width(), last_rect.height() },
                      window });
    }
}

void Taskbar::remove_item(Window& window) {
    for (int i = 0; i < m_items.size(); i++) {
        if (m_items[i].window.get() != &window) {
            continue;
        }

        m_items.remove(i);
        for (; i < m_items.size(); i++) {
            auto& rect_to_move_left = m_items[i].rect;
            m_items[i].rect.set_x(rect_to_move_left.x() - taskbar_item_x_spacing - taskbar_item_width);
        }
        break;
    }
}

void Taskbar::notify_window_added(SharedPtr<Window> window) {
    if (window->visible()) {
        add_item(window);
    }
}

void Taskbar::notify_window_removed(Window& window) {
    if (window.visible()) {
        remove_item(window);
    }
}

void Taskbar::notify_window_visibility_changed(SharedPtr<Window> window) {
    if (window->visible()) {
        add_item(window);
    } else {
        remove_item(*window);
    }
}

bool Taskbar::notify_mouse_pressed(int mouse_x, int mouse_y, mouse_button_state, mouse_button_state) {
    if (mouse_y < m_display_height - taskbar_height) {
        return false;
    }

    for (auto& item : m_items) {
        if (item.rect.intersects({ mouse_x, mouse_y })) {
            WindowManager::the().move_to_front_and_make_active(item.window);
            break;
        }
    }

    return true;
}

void Taskbar::render(Renderer& renderer) {
    auto palette = WindowManager::the().palette();

    int taskbar_top = renderer.pixels().height() - taskbar_height;
    auto taskbar_rect = Rect { 0, taskbar_top, renderer.pixels().width(), taskbar_height };
    renderer.fill_rect(taskbar_rect, palette->color(Palette::TaskbarBackground));
    renderer.draw_line({ 0, taskbar_top }, { renderer.pixels().width() - 1, taskbar_top }, palette->color(Palette::Outline));

    for (int i = 0; i < m_items.size(); i++) {
        auto& item = m_items[i];
        auto& rect = item.rect;
        auto& window = *item.window;
        renderer.draw_rect(rect, palette->color(Palette::Outline));
        renderer.render_text(window.title(), rect.adjusted(-4), palette->color(Palette::Text), TextAlign::CenterLeft,
                             &window == WindowManager::the().active_window() ? Font::bold_font() : Font::default_font());
    }

    time_t now = time(nullptr);
    struct tm* tm = localtime(&now);
    auto time_string = String::format("%2d:%02d:%02d %s", tm->tm_hour % 12, tm->tm_min, tm->tm_sec, tm->tm_hour > 12 ? "PM" : "AM");

    renderer.render_text(time_string, taskbar_rect, palette->color(Palette::Text), TextAlign::CenterRight);
}
