#include <app/application.h>
#include <app/application_os_2.h>
#include <app/window.h>
#include <liim/format.h>
#include <signal.h>

#ifdef USE_SDL2
#include <app/application_sdl.h>
#endif /* USE_SDL2 */

namespace App {
static Application* s_app;

UniquePtr<Application> Application::create() {
#ifdef __os_2__
    return UniquePtr<Application>(new OSApplication);
#elif USE_SDL2
    return UniquePtr<Application>(new SDLApplication);
#else
    error_log("No application backend could be found");
    abort();
#endif
}

Application::Application() {
    s_app = this;
}

Application::~Application() {}

Application& Application::the() {
    return *s_app;
}

void Application::set_global_palette(const String& path) {
    auto palette = Palette::create_from_json(path);
    if (!palette) {
        return;
    }

    m_palette = move(palette);

    Window::for_each_window([&](auto& window) {
        EventLoop::queue_event(window.weak_from_this(), make_unique<ThemeChangeEvent>());
    });
}

void Application::enter() {
    EventLoop::register_signal_handler(SIGINT, [] {
        Application::the().main_event_loop().set_should_exit(true);
    });
    return m_loop.enter();
}
}
