#include <app/scroll_component.h>
#include <app/widget.h>
#include <graphics/renderer.h>

namespace App {
Renderer ScrollComponent::get_renderer() {
    auto renderer = widget().get_renderer();
    renderer.set_bounding_rect(available_rect().with_x(widget().positioned_rect().x()).with_y(widget().positioned_rect().y()));
    renderer.set_translation(-m_scroll_offset);
    return renderer;
}

void ScrollComponent::draw_scrollbars(Renderer&) {}

Rect ScrollComponent::available_rect() {
    return widget().sized_rect().translated(m_scroll_offset);
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

void ScrollComponent::did_attach() {
    widget().intercept<MouseScrollEvent>({}, [this](const MouseScrollEvent& event) {
        m_scroll_offset.set_y(m_scroll_offset.y() + event.z() * 6);
        clamp_scroll_offset();
        widget().invalidate();
        return true;
    });
}

void ScrollComponent::clamp_scroll_offset() {
    m_scroll_offset.set_x(clamp(m_scroll_offset.x(), 0, max(0, total_rect().width() - available_rect().width())));
    m_scroll_offset.set_y(clamp(m_scroll_offset.y(), 0, max(0, total_rect().height() - available_rect().height())));
}
}
