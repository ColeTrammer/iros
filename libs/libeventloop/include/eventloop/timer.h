#pragma once

#include <eventloop/object.h>
#include <liim/function.h>
#include <time.h>

namespace App {

class Timer : public Object {
    APP_OBJECT(Timer)

public:
    static SharedPtr<Timer> create_interval_timer(SharedPtr<Object> parent, Function<void(int)> callback, time_t ms);
    static SharedPtr<Timer> create_single_shot_timer(SharedPtr<Object> parent, Function<void(int)> callback, time_t ms);

    Timer();
    virtual ~Timer() override;

    bool expired() const { return m_expired; }
    bool single_shot() const { return m_single_shot; }

    void set_timeout(itimerspec timeout);
    void set_timeout(time_t ms);
    void set_interval(time_t ms);

    virtual void on_event(Event&);

    Function<void(int)> on_timeout;

private:
    void maybe_setup();

    timer_t m_timer_id { (timer_t) -1 };
    bool m_expired { false };
    bool m_setup { false };
    bool m_single_shot { false };
};

}
