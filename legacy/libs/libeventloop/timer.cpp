#include <eventloop/event.h>
#include <eventloop/event_loop.h>
#include <eventloop/timer.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>

namespace App {

SharedPtr<Timer> Timer::create_interval_timer(Object* parent, time_t ms) {
    auto ret = Timer::create(parent);
    if (!ret) {
        return ret;
    }
    ret->set_interval(ms);
    return ret;
}

SharedPtr<Timer> Timer::create_single_shot_timer(Object* parent, time_t ms) {
    auto ret = Timer::create(parent);
    if (!ret) {
        return ret;
    }
    ret->set_timeout(ms);
    return ret;
}

Timer::Timer() {}

void Timer::initialize() {
    on<TimerEvent>([this](auto&) {
        if (m_single_shot) {
            m_expired = true;
        }
    });

    Object::initialize();
}

Timer::~Timer() {
    if (m_timer_id != (timer_t) -1) {
        EventLoop::unregister_timer_callback(m_timer_id);
        timer_delete(m_timer_id);
    }
}

void Timer::maybe_setup() {
    if (m_setup) {
        return;
    }
    m_setup = true;

    sigevent ev;
    EventLoop::setup_timer_sigevent(ev, &m_timer_id);
    int ret = timer_create(CLOCK_MONOTONIC, &ev, &m_timer_id);
    if (ret == 0) {
        EventLoop::register_timer_callback(m_timer_id, weak_from_this());
        assert(m_timer_id != (timer_t) -1);
    }
}

void Timer::set_timeout(itimerspec timeout) {
    maybe_setup();

    if (m_timer_id != (timer_t) -1) {
        m_expired = false;
        m_single_shot = timeout.it_interval.tv_sec == 0 && timeout.it_interval.tv_nsec == 0;
        timer_settime(m_timer_id, 0, &timeout, nullptr);
    }
}

void Timer::set_timeout(time_t ms) {
    itimerspec spec;
    spec.it_value.tv_sec = ms / 1000;
    spec.it_value.tv_nsec = (ms % 1000) * 1000000;
    spec.it_interval.tv_sec = 0;
    spec.it_interval.tv_nsec = 0;
    set_timeout(spec);
}

void Timer::set_interval(time_t ms) {
    itimerspec spec;
    spec.it_value.tv_sec = ms / 1000;
    spec.it_value.tv_nsec = (ms % 1000) * 1000000;
    spec.it_interval.tv_sec = spec.it_value.tv_sec;
    spec.it_interval.tv_nsec = spec.it_value.tv_nsec;
    set_timeout(spec);
}
}
