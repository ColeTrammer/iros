#pragma once

#include <app/forward.h>
#include <eventloop/component.h>
#include <graphics/forward.h>
#include <graphics/point.h>

namespace App {
enum ScrollDirection {
    Horizontal = 1 << 0,
    Vertiacal = 1 << 1,
};

class ScrollComponent : public Component {
public:
    static constexpr int scrollbar_width = 16;

    explicit ScrollComponent(Object& object) : Component(object) {}
    virtual ~ScrollComponent() override {}

    Renderer get_renderer();
    void draw_scrollbars();

    Rect available_rect();
    Point scroll_offset() { return m_scroll_offset; }

    bool vertically_scrollable();
    bool horizontally_scrollable();

    bool draw_vertical_scrollbar() { return vertically_scrollable() && !!(m_scrollbar_visibility & ScrollDirection::Vertiacal); }
    bool draw_horizontal_scrollbar() { return horizontally_scrollable() && !!(m_scrollbar_visibility & ScrollDirection::Vertiacal); }

    void set_scrollability(int flags) {
        m_scrollability = flags;
        clamp_scroll_offset();
    }

    void set_scrollbar_visibility(int flags) {
        m_scrollbar_visibility = flags;
        clamp_scroll_offset();
    }

    Rect total_rect();

protected:
    virtual void did_attach() override;

private:
    Widget& widget() { return typed_object<Widget>(); }
    void clamp_scroll_offset();

    Point m_scroll_offset;
    int m_scrollability { ScrollDirection::Vertiacal };
    int m_scrollbar_visibility { ScrollDirection::Horizontal | ScrollDirection::Vertiacal };
};
}
