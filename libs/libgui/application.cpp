#include <app/layout_engine.h>
#include <gui/application.h>
#include <gui/application_os_2.h>
#include <gui/window.h>
#include <liim/format.h>
#include <signal.h>

#ifdef USE_SDL2
#include <gui/application_sdl.h>
#endif /* USE_SDL2 */

namespace GUI {
SharedPtr<Application> Application::create() {
#ifdef __os_2__
    return OSApplication::create(nullptr);
#elif USE_SDL2
    return SDLApplication::create(nullptr);
#else
    error_log("No application backend could be found");
    abort();
#endif
}

Application& Application::the() {
    return static_cast<Application&>(App::Application::the());
}

Application::Application() {}

Application::~Application() {}

App::Margins Application::default_margins() const {
    return { 5, 5, 5, 5 };
}

int Application::default_spacing() const {
    return 2;
}

void Application::set_global_palette(const String& path) {
    auto palette = Palette::create_from_json(path);
    if (!palette) {
        return;
    }

    m_palette = move(palette);

    Window::for_each_window([&](auto& window) {
        App::EventLoop::queue_event(window.weak_from_this(), make_unique<App::ThemeChangeEvent>());
    });
}

void Application::before_enter() {
    App::EventLoop::register_signal_handler(SIGINT, [] {
        Application::the().main_event_loop().set_should_exit(true);
    });
}
}
