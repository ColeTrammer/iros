#include <app/event_loop.h>
#include <app/selectable.h>
#include <assert.h>

namespace App {

Selectable::~Selectable() {
    disable_notifications();
}

void Selectable::enable_notifications() {
    if (m_notifications_enabled) {
        return;
    }

    assert(m_fd != -1);
    EventLoop::register_selectable(*this);
}

void Selectable::disable_notifications() {
    if (!m_notifications_enabled) {
        return;
    }

    EventLoop::unregister_selectable(*this);
}

}
