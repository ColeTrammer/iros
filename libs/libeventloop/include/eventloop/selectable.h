#pragma once

#include <eventloop/event.h>
#include <eventloop/object.h>
#include <liim/function.h>

APP_EVENT(App, ReadableEvent, Event, (), (), ())
APP_EVENT(App, WritableEvent, Event, (), (), ())
APP_EVENT(App, ExceptionalEvent, Event, (), (), ())

namespace App {
enum NotifyWhen {
    Readable = 1,
    Writeable = 2,
    Exceptional = 4,
};

class Selectable : public Object {
    APP_OBJECT(Selectable)

    APP_EMITS(Object, ReadableEvent, WritableEvent, ExceptionalEvent)

public:
    Selectable();
    virtual ~Selectable();

    void set_selected_events(int events) { m_selected_events = events; }
    int selected_events() const { return m_selected_events; }

    void enable_notifications();
    void disable_notifications();

    int fd() const { return m_fd; }
    bool valid() const { return m_fd != -1; }

protected:
    void set_fd(int fd) { m_fd = fd; }

private:
    int m_selected_events { 0 };
    int m_fd { -1 };
    bool m_notifications_enabled { false };
};

class FdWrapper final : public Selectable {
    APP_OBJECT(FdWrapper)

public:
    explicit FdWrapper(int fd) { set_fd(fd); }
};
}
