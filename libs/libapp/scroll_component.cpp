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

void ScrollComponent::draw_scrollbars() {
    auto renderer = widget().get_renderer();

    if (horizontally_scrollable()) {
        renderer.fill_rect({ 0, widget().sized_rect().height() - scrollbar_width, widget().sized_rect().width(), scrollbar_width },
                           widget().background_color());

        double percent_scrolled_x = m_scroll_offset.x() / static_cast<double>(total_rect().width() - available_rect().width());

        int fixed_width = 30;
        int bar_offset = ceil(widget().sized_rect().width() - fixed_width) * percent_scrolled_x;
        renderer.fill_rect(
            {
                bar_offset,
                widget().sized_rect().height() - scrollbar_width,
                fixed_width,
                scrollbar_width,
            },
            ColorValue::White);
    }

    if (vertically_scrollable()) {
        renderer.fill_rect({ widget().sized_rect().width() - scrollbar_width, 0, scrollbar_width, widget().sized_rect().height() },
                           widget().background_color());

        double percent_scrolled_y = m_scroll_offset.y() / static_cast<double>(total_rect().height() - available_rect().height());

        int fixed_height = 30;
        int bar_offset = ceil(widget().sized_rect().height() - fixed_height) * percent_scrolled_y;
        renderer.fill_rect(
            {
                widget().sized_rect().width() - scrollbar_width,
                bar_offset,
                scrollbar_width,
                fixed_height,
            },
            ColorValue::White);
    }
}

Rect ScrollComponent::available_rect() {
    return widget()
        .sized_rect()
        .translated(m_scroll_offset)
        .shrinked(vertically_scrollable() ? scrollbar_width : 0, horizontally_scrollable() ? scrollbar_width : 0);
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
    return !!(m_scrollability & Scrollability::Vertiacal) && total_rect().height() > widget().sized_rect().height();
}

bool ScrollComponent::horizontally_scrollable() {
    return !!(m_scrollability & Scrollability::Horizontal) && total_rect().width() > widget().sized_rect().width();
}

void ScrollComponent::did_attach() {
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
