#pragma once

#include <app/base/forward.h>
#include <app/base/scroll_component_bridge.h>
#include <app/base/scroll_component_interface.h>
#include <eventloop/component.h>
#include <graphics/forward.h>
#include <graphics/point.h>

namespace App::Base {
enum ScrollDirection {
    Horizontal = 1 << 0,
    Vertiacal = 1 << 1,
};

class ScrollComponent {
    APP_BASE_SCROLL_COMPONENT_BRIDGE_INTERFACE_FORWARD(bridge())

public:
    explicit ScrollComponent(Widget& widget, SharedPtr<ScrollComponentBridge> bridge);
    void initialize();
    virtual ~ScrollComponent();

    // os_2 reflect begin
    Rect available_rect();
    Point scroll_offset() { return m_scroll_offset; }

    bool vertically_scrollable();
    bool horizontally_scrollable();

    bool draw_vertical_scrollbar() { return vertically_scrollable() && !!(m_scrollbar_visibility & ScrollDirection::Vertiacal); }
    bool draw_horizontal_scrollbar() { return horizontally_scrollable() && !!(m_scrollbar_visibility & ScrollDirection::Horizontal); }

    void set_scrollability(int flags) {
        m_scrollability = flags;
        clamp_scroll_offset();
    }

    void set_scrollbar_visibility(int flags) {
        m_scrollbar_visibility = flags;
        clamp_scroll_offset();
    }

    Rect total_rect();
    // os_2 reflect end

private:
    ScrollComponentBridge& bridge() { return *m_bridge; }
    const ScrollComponentBridge& bridge() const { return *m_bridge; }

    Widget& widget() { return m_widget; }
    void clamp_scroll_offset();

    Point m_scroll_offset;
    Widget& m_widget;
    SharedPtr<ScrollComponentBridge> m_bridge;
    int m_scrollability { ScrollDirection::Vertiacal };
    int m_scrollbar_visibility { ScrollDirection::Horizontal | ScrollDirection::Vertiacal };
};
}
