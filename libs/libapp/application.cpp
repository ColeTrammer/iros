#include <app/application.h>
#include <app/application_os_2.h>
#include <app/application_sdl.h>

namespace App {
static Application* s_app;

UniquePtr<Application> Application::create() {
#ifdef __os_2__
    return UniquePtr<Application>(new OSApplication);
#elif __linux__
    return UniquePtr<Application>(new SDLApplication);
#else
#error "No platform specific application implementation"
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
}
}
