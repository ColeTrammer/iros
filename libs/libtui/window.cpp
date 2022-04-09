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

void Window::do_render() {
    auto& io_terminal = TUI::Application::the().io_terminal();
    io_terminal.set_show_cursor(false);
    main_widget().render_including_children();

    if (auto panel = focused_widget()) {
        if (auto cursor_position = panel->cursor_position()) {
            auto translated = cursor_position->translated(panel->positioned_rect().top_left());
            if (io_terminal.terminal_rect().intersects(translated) && panel->positioned_rect().intersects(translated)) {
                io_terminal.move_cursor_to(translated);
                io_terminal.set_show_cursor(true);
            }
        }
    }

    io_terminal.flush();

    clear_dirty_rects();
}
}
