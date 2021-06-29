#pragma once

#include <app/widget.h>
#include <eventloop/timer.h>
#include <graphics/rect.h>
#include <liim/pointers.h>
#include <liim/vector.h>

namespace Taskbar {
constexpr int taskbar_height = 32;
constexpr int taskbar_item_x_spacing = 8;
constexpr int taskbar_item_y_spacing = 4;
constexpr int taskbar_item_width = 128;
constexpr int taskbar_button_width = 16;
constexpr int taskbar_button_x_margin = 12;
constexpr int taskbar_button_y_margin = 8;

class Taskbar final
    : public App::Widget
    , public App::WindowServerListener {
    APP_OBJECT(Taskbar)

public:
    Taskbar();
    virtual void initialize() override;
    virtual ~Taskbar() override;

    virtual void on_mouse_down(const App::MouseEvent& event) override;

    virtual void render() override;

private:
    struct TaskbarItem {
        Rect rect;
        String title;
        int wid { 0 };
        bool active { false };
    };

    virtual void server_did_create_window(const WindowServer::Server::ServerDidCreatedWindow& data) override;
    virtual void server_did_change_window_title(const WindowServer::Server::ServerDidChangeWindowTitle& data) override;
    virtual void server_did_close_window(const WindowServer::Server::ServerDidCloseWindow& data) override;
    virtual void server_did_make_window_active(const WindowServer::Server::ServerDidMakeWindowActive& data) override;

    TaskbarItem* find_by_wid(int wid);

    void add_item(String title, int wid, bool active);
    void remove_item(int wid);

    Vector<TaskbarItem> m_items;
    SharedPtr<App::Timer> m_time_timer;
    Rect m_button_rect;
};
}
