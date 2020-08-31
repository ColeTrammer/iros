#pragma once

#include <app/object.h>
#include <liim/function.h>
#include <time.h>

namespace App {

class Timer : public Object {
    APP_OBJECT(Timer)

public:
    static SharedPtr<Timer> create_interval_timer(SharedPtr<Object> parent, Function<void()> callback, time_t ms);
    static SharedPtr<Timer> create_single_shot_timer(SharedPtr<Object> parent, Function<void()> callback, time_t ms);

    Timer(Function<void()> callback);
    virtual ~Timer() override;

    void set_timeout(itimerspec timeout);

    Function<void()> on_timeout;

private:
    timer_t m_timer_id { (timer_t) -1 };
};

}
