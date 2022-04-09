#include <tinput/io_terminal.h>
#include <tui/application.h>
#include <tui/window.h>

namespace TUI {
void Window::initialize() {
    on_unchecked<App::KeyDownEvent>([this](const App::KeyDownEvent& event) {
        if (Application::the().quit_on_control_q() && event.control_down() && event.key() == App::Key::Q) {
            Application::the().main_event_loop().set_should_exit(true);
            return true;
        }
        return false;
    });

    App::Window::initialize();
}

Window* Window::parent_window() {
    return static_cast<Window*>(App::Window::parent_window());
}

void Window::schedule_render() {
    TUI::Application::the().schedule_render();
}

void Window::render_subwindows() {
    for (auto& child : children()) {
        if (child->is_window()) {
            static_cast<Window&>(const_cast<Object&>(*child)).do_render();
        }
    }
}

void Window::do_render() {
    flush_layout();

    main_widget().render_including_children();

    clear_dirty_rects();

    render_subwindows();
}
}
