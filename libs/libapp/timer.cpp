#include <app/event_loop.h>
#include <app/timer.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>

namespace App {

SharedPtr<Timer> Timer::create_interval_timer(SharedPtr<Object> parent, Function<void()> callback, time_t ms) {
    auto ret = Timer::create(move(parent), move(callback));
    if (!ret) {
        return ret;
    }

    itimerspec spec;
    spec.it_value.tv_sec = ms / 1000;
    spec.it_value.tv_nsec = (ms % 1000) * 1000000;
    spec.it_interval.tv_sec = spec.it_value.tv_sec;
    spec.it_interval.tv_nsec = spec.it_value.tv_nsec;
    ret->set_timeout(spec);
    return ret;
}

SharedPtr<Timer> Timer::create_single_shot_timer(SharedPtr<Object> parent, Function<void()> callback, time_t ms) {
    auto ret = Timer::create(move(parent), move(callback));
    if (!ret) {
        return ret;
    }

    itimerspec spec;
    spec.it_value.tv_sec = ms / 1000;
    spec.it_value.tv_nsec = (ms % 1000) * 1000000;
    spec.it_interval.tv_sec = 0;
    spec.it_interval.tv_nsec = 0;
    ret->set_timeout(spec);
    return ret;
}

Timer::Timer(Function<void()> callback) {
    sigevent ev;
    EventLoop::setup_timer_sigevent(ev, &m_timer_id);
    int ret = timer_create(CLOCK_MONOTONIC, &ev, &m_timer_id);
    if (ret == 0) {
        EventLoop::register_timer_callback(m_timer_id, move(callback));
        assert(m_timer_id != (timer_t) -1);
    }
}

Timer::~Timer() {
    if (m_timer_id != (timer_t) -1) {
        EventLoop::unregister_timer_callback(m_timer_id);
        timer_delete(m_timer_id);
    }
}

void Timer::set_timeout(itimerspec timeout) {
    if (m_timer_id != (timer_t) -1) {
        timer_settime(m_timer_id, 0, &timeout, nullptr);
    }
}

}
