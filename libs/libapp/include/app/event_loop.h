#pragma once

#include <liim/function.h>
#include <liim/pointers.h>
#include <liim/vector.h>

namespace App {

class Event;
class Object;
class Selectable;

class EventLoop {
public:
    EventLoop();
    ~EventLoop();

    EventLoop(const EventLoop&) = delete;

    static void register_selectable(Selectable& selectable);
    static void unregister_selectable(Selectable& selectable);
    static void register_signal_handler(int signum, Function<void()> callback);
    static void queue_event(WeakPtr<Object> target, UniquePtr<Event> event);

    void enter();
    void set_should_exit(bool b) { m_should_exit = b; }

private:
    void do_select();
    void do_event_dispatch();
    void setup_signal_handlers();

    struct QueuedEvent {
        WeakPtr<Object> target;
        UniquePtr<Event> event;
    };

    Vector<QueuedEvent> m_events;
    bool m_should_exit { false };
};

}
