#pragma once

#include <app/object.h>
#include <liim/maybe.h>
#include <liim/string.h>
#include <window_server/window.h>

namespace App {

class Widget;

class Window : public Object {
    APP_OBJECT(Window)

public:
    static Maybe<SharedPtr<Window>> find_by_wid(wid_t wid);

    virtual ~Window();

    SharedPtr<PixelBuffer>& pixels() { return m_ws_window->pixels(); }

    wid_t wid() const { return m_ws_window->wid(); }

    void set_focused_widget(Widget* widget);
    SharedPtr<Widget> focused_widget();

    void invalidate_rect(const Rect& rect);

    template<typename T, typename... Args>
    T& set_main_widget(Args... args) {
        auto ret = T::create(shared_from_this(), forward<Args>(args)...);
        ret->set_rect(rect());
        set_focused_widget(ret.get());
        invalidate_rect(rect());
        m_main_widget = ret;
        return *ret;
    }

    const Rect& rect() const { return m_rect; }
    void set_rect(const Rect& rect);

    void hide();
    void show();
    bool visible() const { return m_visible; }

protected:
    Window(int x, int y, int width, int height, String name, WindowServer::WindowType window_type = WindowServer::WindowType::Application,
           wid_t parent_id = 0);
    virtual void on_event(Event& event) override;

private:
    virtual bool is_window() const final { return true; }

    Widget* find_widget_at_point(Point p);
    void draw();

    static void register_window(Window& window);
    static void unregister_window(wid_t wid);

    SharedPtr<WindowServer::Window> m_ws_window;
    WeakPtr<Widget> m_focused_widget;
    SharedPtr<Widget> m_main_widget;
    Rect m_rect;
    bool m_will_draw_soon { false };
    bool m_left_down { false };
    bool m_right_down { false };
    bool m_visible { true };
};

}
