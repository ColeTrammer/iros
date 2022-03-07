#include <app/base/scroll_component.h>
#include <app/base/widget.h>
#include <app/widget.h>
#include <graphics/renderer.h>
#include <math.h>

namespace App::Base {
ScrollComponent::ScrollComponent(Widget& widget, SharedPtr<ScrollComponentBridge> bridge) : m_widget(widget), m_bridge(move(bridge)) {}

ScrollComponent::~ScrollComponent() {}

Rect ScrollComponent::available_rect() {
    auto scrollbar_width = this->scrollbar_width();
    return widget()
        .sized_rect()
        .translated(m_scroll_offset)
        .shrinked(draw_vertical_scrollbar() ? scrollbar_width : 0, draw_horizontal_scrollbar() ? scrollbar_width : 0);
}

Rect ScrollComponent::total_rect() {
    auto& hints = widget().layout_constraint();
    return {
        0,
        0,
        hints.width() == LayoutConstraint::AutoSize ? widget().sized_rect().width() : hints.width(),
        hints.height() == LayoutConstraint::AutoSize ? widget().sized_rect().height() : hints.height(),
    };
}

bool ScrollComponent::vertically_scrollable() {
    return !!(m_scrollability & ScrollDirection::Vertiacal) && total_rect().height() > widget().sized_rect().height();
}

bool ScrollComponent::horizontally_scrollable() {
    return !!(m_scrollability & ScrollDirection::Horizontal) && total_rect().width() > widget().sized_rect().width();
}

void ScrollComponent::initialize() {
    widget().intercept<MouseScrollEvent>({}, [this](const MouseScrollEvent& event) {
        m_scroll_offset.set_y(m_scroll_offset.y() + event.z() * 6);
        clamp_scroll_offset();
        widget().invalidate();
        return true;
    });

    widget().intercept<ResizeEvent>({}, [this](auto&) {
        clamp_scroll_offset();
    });
}

void ScrollComponent::clamp_scroll_offset() {
    if (!horizontally_scrollable()) {
        m_scroll_offset.set_x(0);
    } else {
        m_scroll_offset.set_x(clamp(m_scroll_offset.x(), 0, max(0, total_rect().width() - available_rect().width())));
    }

    if (!vertically_scrollable()) {
        m_scroll_offset.set_y(0);
    } else {
        m_scroll_offset.set_y(clamp(m_scroll_offset.y(), 0, max(0, total_rect().height() - available_rect().height())));
    }
}
}
