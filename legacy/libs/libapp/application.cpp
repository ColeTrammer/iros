#include <app/application.h>

namespace App {
static Application* s_the;

Application& Application::the() {
    assert(s_the);
    return *s_the;
}

Application::Application() {
    assert(!s_the);
    s_the = this;
}

Application::~Application() {
    s_the = nullptr;
}

void Application::enter() {
    before_enter();
    m_loop.enter();
}
}
