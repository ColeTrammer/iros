#pragma once

#include <app/forward.h>
#include <graphics/rect.h>
#include <liim/pointers.h>

namespace App {
struct Margins {
    int top { 0 };
    int right { 0 };
    int bottom { 0 };
    int left { 0 };
};

class Layout {
public:
    Layout();
    virtual ~Layout() {}

    virtual void do_add(SharedPtr<Widget> widget) = 0;
    virtual void layout() = 0;

    Widget& widget() { return m_widget; }
    const Widget& widget() const { return m_widget; }

    const Rect& container_rect() const;

    void set_margins(const Margins& m) { m_margins = m; }
    const Margins& margins() const { return m_margins; }

    SharedPtr<Widget> widget_pointer();

    template<typename WidgetClass, typename... Args>
    WidgetClass& add(Args&&... args) {
        auto ret = WidgetClass::create(widget_pointer(), forward<Args>(args)...);
        do_add(ret);
        return *ret;
    }

protected:
    Layout(Widget& widget) : m_widget(widget) {}

private:
    Widget& m_widget;
    Margins m_margins { 5, 5, 5, 5 };
};
}
