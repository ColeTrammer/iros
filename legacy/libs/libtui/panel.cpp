#include <eventloop/event.h>
#include <eventloop/key_bindings.h>
#include <liim/option.h>
#include <tinput/terminal_renderer.h>
#include <tui/application.h>
#include <tui/panel.h>

namespace TUI {
Panel::Panel() {}

void Panel::did_attach() {
    on<App::FocusedEvent>([this](auto&) {
        if (cursor_position().has_value()) {
            if (auto* window = parent_window()) {
                window->schedule_render();
            }
        }
    });
}

Panel::~Panel() {}

TInput::TerminalRenderer Panel::get_renderer() {
    auto dirty_rects = TUI::Application::the().root_window().dirty_rects();
    Function<void(App::Widget&)> enumerate_children = [&](App::Widget& widget) {
        for (auto& child : widget.children()) {
            if (child->is_base_widget()) {
                auto& widget = const_cast<App::Widget&>(static_cast<const App::Widget&>(*child));
                dirty_rects.subtract(widget.positioned_rect());
                enumerate_children(widget);
            }
        }
    };
    enumerate_children(base());

    auto renderer = TInput::TerminalRenderer { Application::the().io_terminal(), dirty_rects };
    renderer.set_clip_rect(positioned_rect());
    renderer.set_origin(positioned_rect().top_left());
    return renderer;
}
}
