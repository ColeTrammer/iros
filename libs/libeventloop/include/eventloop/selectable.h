#pragma once

#include <eventloop/object.h>
#include <liim/function.h>

namespace App {

enum NotifyWhen {
    Readable = 1,
    Writeable = 2,
    Exceptional = 4,
};

class Selectable : public Object {
public:
    virtual ~Selectable();

    void set_selected_events(int events) { m_selected_events = events; }
    int selected_events() const { return m_selected_events; }

    void enable_notifications();
    void disable_notifications();

    int fd() const { return m_fd; }
    bool valid() const { return m_fd != -1; }

    virtual void notify_readable() {}
    virtual void notify_writeable() {}
    virtual void notify_exceptional() {}

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
    FdWrapper(int fd) { set_fd(fd); }

    Function<void()> on_readable;

    virtual void notify_readable() override {
        if (on_readable) {
            on_readable();
        }
    }
};

}
