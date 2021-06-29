#include <app/app.h>
#include <app/window.h>
#include <graphics/palette.h>
#include <graphics/renderer.h>
#include <spawn.h>

#include "taskbar.h"

namespace Taskbar {
Taskbar::Taskbar() {}

void Taskbar::initialize() {
    m_time_timer = App::Timer::create_interval_timer(
        shared_from_this(),
        [this](int) {
            invalidate();
        },
        1000);

    m_button_rect = { taskbar_button_x_margin, taskbar_button_y_margin, taskbar_button_width,
                      taskbar_height - 2 * taskbar_button_y_margin };
}

Taskbar::~Taskbar() {}

void Taskbar::add_item(String title, int wid, bool active) {
    if (m_items.empty()) {
        m_items.add({ { taskbar_button_x_margin * 2 + taskbar_button_width, taskbar_item_y_spacing, taskbar_item_width,
                        taskbar_height - 2 * taskbar_item_y_spacing },
                      move(title),
                      wid,
                      active });
    } else {
        auto& last_rect = m_items.last().rect;
        m_items.add({ { last_rect.x() + last_rect.width() + taskbar_item_x_spacing, last_rect.y(), last_rect.width(), last_rect.height() },
                      move(title),
                      wid,
                      active });
    }
}

void Taskbar::remove_item(int wid) {
    for (int i = 0; i < m_items.size(); i++) {
        if (m_items[i].wid != wid) {
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

Taskbar::TaskbarItem* Taskbar::find_by_wid(int wid) {
    for (auto& item : m_items) {
        if (item.wid == wid) {
            return &item;
        }
    }
    return nullptr;
}

void Taskbar::server_did_create_window(const WindowServer::Server::ServerDidCreatedWindow& data) {
    if (data.type != WindowServer::WindowType::Application) {
        return;
    }

    add_item(data.title, data.wid, true);
    invalidate();
}

void Taskbar::server_did_change_window_title(const WindowServer::Server::ServerDidChangeWindowTitle& data) {
    auto* item = find_by_wid(data.wid);
    if (!item) {
        return;
    }

    item->title = data.new_title;
    invalidate();
}

void Taskbar::server_did_close_window(const WindowServer::Server::ServerDidCloseWindow& data) {
    remove_item(data.wid);
    invalidate();
}

void Taskbar::server_did_make_window_active(const WindowServer::Server::ServerDidMakeWindowActive& data) {
    for (auto& item : m_items) {
        item.active = false;
    }
    invalidate();

    auto* item = find_by_wid(data.wid);
    if (!item) {
        return;
    }

    item->active = true;
}

void Taskbar::on_mouse_event(App::MouseEvent& event) {
    if (m_button_rect.intersects({ event.x(), event.y() })) {
        if (event.left() == MOUSE_DOWN) {
            char* const args[] = { (char*) "terminal", nullptr };
            posix_spawnp(nullptr, args[0], nullptr, nullptr, args, environ);
        }
        return;
    }

    for (auto& item : m_items) {
        if (item.rect.intersects({ event.x(), event.y() })) {
            if (event.left() == MOUSE_DOWN) {
                App::App::the().ws().server().send<WindowServer::Client::SetActiveWindow>({ item.wid });
                return;
            }
        }
    }

    App::Widget::on_mouse_event(event);
}

void Taskbar::render() {
    auto renderer = get_renderer();

    auto& palette = *App::App::the().palette();

    auto taskbar_rect = Rect { 0, 0, renderer.pixels().width(), taskbar_height };
    renderer.fill_rect(taskbar_rect, palette.color(Palette::TaskbarBackground));
    renderer.draw_line({ 0, 0 }, { renderer.pixels().width() - 1, 0 }, palette.color(Palette::Outline));

    renderer.fill_rect(m_button_rect, palette.color(Palette::Text));

    for (int i = 0; i < m_items.size(); i++) {
        auto& item = m_items[i];
        auto& rect = item.rect;
        renderer.draw_rect(rect, palette.color(Palette::Outline));
        renderer.render_text(item.title, rect.adjusted(-4), palette.color(Palette::Text), TextAlign::CenterLeft,
                             item.active ? Font::bold_font() : Font::default_font());
    }

    time_t now = time(nullptr);
    struct tm* tm = localtime(&now);
    auto time_string = String::format("%2d:%02d:%02d %s", tm->tm_hour % 12, tm->tm_min, tm->tm_sec, tm->tm_hour > 12 ? "PM" : "AM");

    renderer.render_text(time_string, taskbar_rect, palette.color(Palette::Text), TextAlign::CenterRight);
}
}
