#pragma once

#include <app/widget.h>
#include <liim/maybe.h>
#include <liim/string.h>
#include <window_server/window.h>

namespace App {

class Window : public Widget {
    APP_OBJECT(Window)

public:
    static Maybe<SharedPtr<Window>> find_by_wid(wid_t wid);

    virtual ~Window();

    void draw() { m_ws_window->draw(); }

    SharedPtr<PixelBuffer>& pixels() { return m_ws_window->pixels(); }

    wid_t wid() const { return m_ws_window->wid(); }

    void set_focused_widget(Widget& widget);

protected:
    Window(int x, int y, int width, int height, String name);
    virtual void on_event(Event& event) override;

private:
    virtual bool is_window() const final { return true; }

    Widget& find_widget_at_point(Point p);

    SharedPtr<Widget> focused_widget();

    static void register_window(Window& window);
    static void unregister_window(wid_t wid);

    SharedPtr<WindowServer::Window> m_ws_window;
    WeakPtr<Widget> m_focused_widget;
};

}
