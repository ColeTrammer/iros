#include <app/app.h>
#include <app/box_layout.h>
#include <app/window.h>

#include "terminal_widget.h"

int main(int, char**) {
    App::App app;

    signal(SIGTTOU, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGWINCH, SIG_IGN);

    auto window = App::Window::create(nullptr, 200, 200, 80 * 8 + 10, 25 * 16 + 10, "Terminal");
    auto& layout = window->set_layout<App::VerticalBoxLayout>();

    auto& terminal_widget = layout.add<TerminalWidget>();
    window->draw();
    window->set_focused_widget(terminal_widget);
    app.enter();
    return 0;
}
