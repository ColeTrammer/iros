#include <app/event.h>
#include <app/event_loop.h>
#include <app/selectable.h>
#include <assert.h>
#include <errno.h>
#include <liim/vector.h>
#include <sys/select.h>

namespace App {

static Vector<Selectable*> s_selectables;

void EventLoop::register_selectable(Selectable& selectable) {
    s_selectables.add(&selectable);
}

void EventLoop::unregister_selectable(Selectable& selectable) {
    s_selectables.remove_element(&selectable);
}

EventLoop::EventLoop() {}

EventLoop::~EventLoop() {}

void EventLoop::queue_event(UniquePtr<Event> event) {
    m_events.add(move(event));
}

void EventLoop::do_select() {
    if (s_selectables.empty()) {
        return;
    }

    fd_set rd_set;
    fd_set wr_set;
    fd_set ex_set;

    FD_ZERO(&rd_set);
    FD_ZERO(&wr_set);
    FD_ZERO(&ex_set);

    for (auto* selectable : s_selectables) {
        int notify_flags = selectable->selected_events();
        assert(selectable->fd() != -1);

        if (notify_flags & NotifyWhen::Readable) {
            FD_SET(selectable->fd(), &rd_set);
        }
        if (notify_flags & NotifyWhen::Writeable) {
            FD_SET(selectable->fd(), &wr_set);
        }
        if (notify_flags & NotifyWhen::Exceptional) {
            FD_SET(selectable->fd(), &ex_set);
        }
    }

    for (;;) {
        int ret = select(FD_SETSIZE, &rd_set, &wr_set, &ex_set, nullptr);
        if (ret == -1) {
            if (errno == EINTR) {
                continue;
            }
            assert(false);
        }

        if (ret == 0) {
            return;
        }
        break;
    }

    for (auto* selectable : s_selectables) {
        int fd = selectable->fd();
        if (FD_ISSET(fd, &rd_set)) {
            selectable->notify_readable();
        }
        if (FD_ISSET(fd, &wr_set)) {
            selectable->notify_writeable();
        }
        if (FD_ISSET(fd, &ex_set)) {
            selectable->notify_exceptional();
        }
    }
}

void EventLoop::do_event_dispatch() {
    for (;;) {
        auto events = move(m_events);
        if (events.empty()) {
            return;
        }

        for (auto& event : events) {
            // dispatch events somewhere.
            (void) event;
        }
    }
}

void EventLoop::enter() {
    for (;;) {
        if (m_should_exit) {
            return;
        }

        do_select();
        do_event_dispatch();
    }
}

}
