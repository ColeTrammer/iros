#include <math.h>
#include <tinput/terminal_renderer.h>
#include <tui/panel.h>
#include <tui/scroll_component.h>

namespace TUI {
ScrollComponent::ScrollComponent(Panel& panel) : m_panel(panel) {}

void ScrollComponent::did_attach() {}

ScrollComponent::~ScrollComponent() {}

TInput::TerminalRenderer ScrollComponent::get_renderer() {
    auto renderer = panel().get_renderer();
    renderer.set_clip_rect(available_rect().with_x(panel().positioned_rect().x()).with_y(panel().positioned_rect().y()));
    renderer.set_origin(-scroll_offset());
    return renderer;
}

void ScrollComponent::draw_scrollbars() {
    auto renderer = panel().get_renderer();
    auto scrollbar_width = this->scrollbar_width();

    if (draw_horizontal_scrollbar()) {
        renderer.clear_rect({ 0, panel().sized_rect().height() - scrollbar_width, panel().sized_rect().width(), scrollbar_width });

        double percent_scrolled_x = scroll_offset().x() / static_cast<double>(total_rect().width() - available_rect().width());

        int fixed_width = 5;
        int bar_offset = ceil(panel().sized_rect().width() - fixed_width) * percent_scrolled_x;

        renderer.clear_rect({ bar_offset, panel().sized_rect().height() - scrollbar_width, fixed_width, scrollbar_width },
                            { ColorValue::White });
    }

    if (draw_vertical_scrollbar()) {
        renderer.clear_rect({ panel().sized_rect().width() - scrollbar_width, 0, scrollbar_width, panel().sized_rect().height() });

        double percent_scrolled_y = scroll_offset().y() / static_cast<double>(total_rect().height() - available_rect().height());

        int fixed_height = 5;
        int bar_offset = ceil(panel().sized_rect().height() - fixed_height) * percent_scrolled_y;
        renderer.clear_rect(
            {
                panel().sized_rect().width() - scrollbar_width,
                bar_offset,
                scrollbar_width,
                fixed_height,
            },
            { ColorValue::White });
    }
}

void ScrollComponent::attach_to_base(App::ScrollComponent& base) {
    m_base = &base;
    did_attach();
}
}
