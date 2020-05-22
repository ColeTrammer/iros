#pragma once

#include <liim/pointers.h>
#include <liim/vector.h>

namespace App {

class Event;
class Selectable;

class EventLoop {
public:
    EventLoop();
    ~EventLoop();

    EventLoop(const EventLoop&) = delete;

    static void register_selectable(Selectable& selectable);
    static void unregister_selectable(Selectable& selectable);

    void enter();
    void queue_event(UniquePtr<Event> event);

    void set_should_exit(bool b) { m_should_exit = b; }

private:
    void do_select();
    void do_event_dispatch();

    Vector<UniquePtr<Event>> m_events;
    bool m_should_exit { false };
};

}
