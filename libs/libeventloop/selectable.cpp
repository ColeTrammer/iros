#include <assert.h>
#include <eventloop/event_loop.h>
#include <eventloop/selectable.h>

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
    m_notifications_enabled = true;
}

void Selectable::disable_notifications() {
    if (!m_notifications_enabled) {
        return;
    }

    EventLoop::unregister_selectable(*this);
    m_notifications_enabled = false;
}

}
