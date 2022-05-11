#pragma once

#include <app/widget.h>
#include <eventloop/event.h>
#include <eventloop/forward.h>
#include <eventloop/key_bindings.h>
#include <graphics/rect_set.h>

APP_EVENT_PARENT(App, WindowEvent, Event, ((StringView, name)), (), ())
APP_EVENT(App, WindowCloseEvent, WindowEvent, (), (), ())
APP_EVENT(App, WindowForceRedrawEvent, WindowEvent, (), (), ())
APP_EVENT(App, WindowDidResizeEvent, WindowEvent, (), (), ())
APP_EVENT(App, WindowStateEvent, WindowEvent, (), ((bool, active)), ())

namespace App {
class Window : public Object {
    APP_OBJECT(Window)

    APP_EMITS(Object, WindowCloseEvent, WindowForceRedrawEvent, WindowDidResizeEvent, WindowStateEvent)

public:
    Window();
    virtual void initialize() override;
    virtual ~Window() override;

    const Rect& rect() const { return m_rect; }
    void set_rect(const Rect& rect);

    void invalidate_rect(const Rect& rect);
    const RectSet& dirty_rects() const { return m_dirty_rects; }
    void clear_dirty_rects() { m_dirty_rects.clear(); }

    void set_key_bindings(KeyBindings key_bindings) { m_key_bindings = move(key_bindings); }

    void set_focused_widget(Widget* widget);
    SharedPtr<Widget> focused_widget();

    template<typename T, typename... Args>
    T& set_main_widget(Args&&... args) {
        auto [result_base, result] = T::create_both_owned(this, forward<Args>(args)...);
        result_base->set_positioned_rect(rect());
        set_focused_widget(result_base.get());
        invalidate_rect(rect());
        m_main_widget = move(result_base);
        return *result;
    }

    Widget& main_widget() { return *m_main_widget; }
    const Widget& main_widget() const { return *m_main_widget; }

    Widget* hit_test(const Widget& root, const Point& point) const;

    Window* parent_window();

    virtual void schedule_render() = 0;

protected:
    virtual void do_render() = 0;

    void flush_layout();
    void set_hovered_widget(Widget* widget);

private:
    virtual bool is_window() const final override { return true; }

    KeyBindings m_key_bindings;
    WeakPtr<Widget> m_focused_widget;
    WeakPtr<Widget> m_hovered_widget;
    SharedPtr<Widget> m_main_widget;
    RectSet m_dirty_rects;
    Rect m_rect;
};
}
