#include <app/application.h>
#include <app/window.h>

#include "taskbar.h"

int main() {
    auto app = App::Application::create();

    auto window = App::Window::create(nullptr, 0, 0, 0, Taskbar::taskbar_height, "Taskbar", true, WindowServer::WindowType::Taskbar);
    auto& taskbar = window->set_main_widget<Taskbar::Taskbar>();

    app->set_window_server_listener(taskbar);

    app->enter();
    return 0;
}
