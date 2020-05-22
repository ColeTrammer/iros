#pragma once

#include <app/object.h>

namespace App {

enum NotifyOn {
    Readable = 1,
    Writeable = 2,
    Exceptional = 4,
};

class Selectable : Object {
public:
    virtual ~Selectable();

    void set_selected_events(int events) { m_selected_events = events; }
    int selected_events() const { return m_selected_events; }

    void enable_notifications();
    void disable_notifications();

    int fd() const { return m_fd; }

protected:
    virtual void notify_readable() {}
    virtual void notify_writeable() {}
    virtual void notify_exceptional() {}

    void set_fd(int fd) { m_fd = fd; }

private:
    int m_selected_events { 0 };
    int m_fd { -1 };
    bool m_notifications_enabled { false };
};

}
