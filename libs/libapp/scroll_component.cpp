#include <app/scroll_component.h>
#include <app/widget.h>
#include <graphics/renderer.h>
#include <math.h>

namespace App {
ScrollComponent::ScrollComponent(Widget& widget) : m_widget(widget) {}

void ScrollComponent::did_attach() {}

ScrollComponent::~ScrollComponent() {}

Renderer ScrollComponent::get_renderer() {
    auto renderer = widget().get_renderer();
    renderer.set_bounding_rect(available_rect().with_x(widget().positioned_rect().x()).with_y(widget().positioned_rect().y()));
    renderer.set_translation(-scroll_offset());
    return renderer;
}

void ScrollComponent::draw_scrollbars() {
    auto renderer = widget().get_renderer();
    auto scrollbar_width = this->scrollbar_width();

    if (draw_horizontal_scrollbar()) {
        renderer.fill_rect({ 0, widget().sized_rect().height() - scrollbar_width, widget().sized_rect().width(), scrollbar_width },
                           widget().background_color());

        double percent_scrolled_x = scroll_offset().x() / static_cast<double>(total_rect().width() - available_rect().width());

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

    if (draw_vertical_scrollbar()) {
        renderer.fill_rect({ widget().sized_rect().width() - scrollbar_width, 0, scrollbar_width, widget().sized_rect().height() },
                           widget().background_color());

        double percent_scrolled_y = scroll_offset().y() / static_cast<double>(total_rect().height() - available_rect().height());

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

void ScrollComponent::attach_to_base(Base::ScrollComponent& base) {
    m_base = &base;
    did_attach();
}
}
