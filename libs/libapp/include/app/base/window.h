#pragma once

#include <app/base/widget.h>
#include <eventloop/forward.h>
#include <graphics/rect_set.h>

namespace App::Base {
class Window : public Object {
    APP_OBJECT(Window)

public:
    Window();
    virtual void initialize() override;
    virtual ~Window() override;

    const Rect& rect() const { return m_rect; }
    void set_rect(const Rect& rect);

    void invalidate_rect(const Rect& rect);
    const RectSet& dirty_rects() const { return m_dirty_rects; }

    void set_focused_widget(Widget* widget);
    SharedPtr<Widget> focused_widget();

    template<typename T, typename... Args>
    T& set_main_widget(Args... args) {
        auto ret = T::create(shared_from_this(), forward<Args>(args)...);
        ret->set_positioned_rect(rect());
        set_focused_widget(ret.get());
        invalidate_rect(rect());
        m_main_widget = ret;
        return *ret;
    }
    Widget& main_widget() { return *m_main_widget; }
    const Widget& main_widget() const { return *m_main_widget; }

    Widget* hit_test(const Widget& root, const Point& point) const;

protected:
    virtual void do_render() = 0;

private:
    void schedule_render();
    void set_hovered_widget(Widget* widget);

    WeakPtr<Widget> m_focused_widget;
    WeakPtr<Widget> m_hovered_widget;
    SharedPtr<Widget> m_main_widget;
    RectSet m_dirty_rects;
    Rect m_rect;
    bool m_render_scheduled { false };
};
}
