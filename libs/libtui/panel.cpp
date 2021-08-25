#include <eventloop/event.h>
#include <eventloop/key_bindings.h>
#include <liim/maybe.h>
#include <tinput/terminal_renderer.h>
#include <tui/application.h>
#include <tui/panel.h>

namespace TUI {
Panel::Panel() {}

void Panel::initialize() {
    on<App::KeyDownEvent>([this](const App::KeyDownEvent& event) {
        if (event.control_down() && event.key() == App::Key::Q) {
            Application::the().main_event_loop().set_should_exit(true);
            return true;
        }
        return false;
    });

    App::Base::Widget::initialize();
}

Panel::~Panel() {}

Panel* Panel::parent_panel() {
    return static_cast<Panel*>(parent_widget());
}

TInput::TerminalRenderer Panel::get_renderer() {
    auto dirty_rects = TUI::Application::the().root_window().dirty_rects();
    Function<void(App::Base::Widget&)> enumerate_children = [&](App::Base::Widget& widget) {
        for (auto& child : widget.children()) {
            if (child->is_base_widget()) {
                auto& widget = const_cast<App::Base::Widget&>(static_cast<const App::Base::Widget&>(*child));
                dirty_rects.subtract(widget.positioned_rect());
                enumerate_children(widget);
            }
        }
    };
    enumerate_children(*this);

    auto renderer = TInput::TerminalRenderer { Application::the().io_terminal(), dirty_rects };
    renderer.set_clip_rect(positioned_rect());
    renderer.set_origin(positioned_rect().top_left());
    return renderer;
}
}
