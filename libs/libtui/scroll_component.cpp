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

void ScrollComponent::draw_scrollbars() {}

void ScrollComponent::attach_to_base(App::ScrollComponent& base) {
    m_base = &base;
    did_attach();
}
}
