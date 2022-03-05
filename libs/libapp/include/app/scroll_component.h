#pragma once

#include <app/forward.h>
#include <eventloop/component.h>
#include <graphics/forward.h>
#include <graphics/point.h>

namespace App {
enum Scrollability {
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

    void set_scrollability(int flags) {
        m_scrollability = flags;
        clamp_scroll_offset();
    }

    Rect total_rect();

protected:
    virtual void did_attach() override;

private:
    Widget& widget() { return typed_object<Widget>(); }
    void clamp_scroll_offset();

    Point m_scroll_offset;
    int m_scrollability { Scrollability::Vertiacal };
};
}
