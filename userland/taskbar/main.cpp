#include <app/app.h>
#include <app/window.h>

#include "taskbar.h"

int main() {
    App::Application app;

    auto window = App::Window::create(nullptr, 0, 0, 0, Taskbar::taskbar_height, "Taskbar", true, WindowServer::WindowType::Taskbar);
    auto& taskbar = window->set_main_widget<Taskbar::Taskbar>();

    app.set_window_server_listener(taskbar);

    app.enter();
    return 0;
}
