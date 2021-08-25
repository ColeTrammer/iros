#include <assert.h>
#include <errno.h>
#include <eventloop/event.h>
#include <eventloop/event_loop.h>
#include <eventloop/selectable.h>
#include <liim/hash_map.h>
#include <liim/vector.h>
#include <signal.h>
#include <sys/select.h>

#define TIMER_SIGNAL         SIGRTMIN + 10
#define WAKEUP_THREAD_SIGNAL SIGRTMIN + 11

#ifdef __linux__
namespace LIIM {

template<>
struct Traits<timer_t> {
    static constexpr bool is_simple() { return true; }
    static unsigned int hash(const timer_t& obj) {
        return static_cast<unsigned int>((uintptr_t) obj) + static_cast<unsigned int>((uintptr_t) obj >> 32);
    };
};

}
#endif /* __linux__ */

namespace App {

static EventLoop* s_the;
static Vector<Selectable*> s_selectables;
static HashMap<int, Function<void()>> s_watched_signals;
static HashMap<timer_t, WeakPtr<Object>> s_timer_objects;
static volatile sig_atomic_t s_signal_number;
static volatile timer_t* s_timer_fired_id;

static void private_signal_handler(int signum) {
    s_signal_number = signum;
}

static void timer_signal_handler(int, siginfo_t* info, void*) {
    s_timer_fired_id = (timer_t*) info->si_value.sival_ptr;
}

static void wakeup_thread_signal_handler(int, siginfo_t*, void*) {}

void EventLoop::register_selectable(Selectable& selectable) {
    s_selectables.add(&selectable);
}

void EventLoop::unregister_selectable(Selectable& selectable) {
    s_selectables.remove_element(&selectable);
}

void EventLoop::register_signal_handler(int signum, Function<void()> callback) {
    // FIXME: allow multiple handlers for the same signal.
    assert(!s_watched_signals.get(signum));
    s_watched_signals.put(signum, move(callback));
}

void EventLoop::unregister_signal_handler(int signum) {
    s_watched_signals.remove(signum);
}

void EventLoop::register_timer_callback(timer_t id, WeakPtr<Object> target) {
    s_timer_objects.put(id, move(target));
}

void EventLoop::unregister_timer_callback(timer_t id) {
    s_timer_objects.remove(id);
}

void EventLoop::setup_timer_sigevent(sigevent& ev, timer_t* id) {
    ev.sigev_notify = SIGEV_SIGNAL;
    ev.sigev_signo = TIMER_SIGNAL;
    ev.sigev_value.sival_ptr = id;
}

void EventLoop::queue_event(WeakPtr<Object> target, UniquePtr<Event> event) {
    s_the->do_queue_event(move(target), move(event));
}

void EventLoop::do_queue_event(WeakPtr<Object> target, UniquePtr<Event> event) {
    pthread_mutex_lock(&m_lock);
    m_events.add({ move(target), move(event) });
    pthread_mutex_unlock(&m_lock);

    if (m_waiting_thread != 0 && pthread_self() != m_waiting_thread) {
        pthread_kill(m_waiting_thread, WAKEUP_THREAD_SIGNAL);
    }
}

EventLoop::EventLoop() {
    assert(!s_the);
    s_the = this;

    pthread_mutex_init(&m_lock, nullptr);
}

EventLoop::~EventLoop() {
    pthread_mutex_destroy(&m_lock);
    s_the = nullptr;
}

void EventLoop::do_select() {
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

    sigset_t sigset;
    sigprocmask(0, nullptr, &sigset);
    sigdelset(&sigset, TIMER_SIGNAL);
    sigdelset(&sigset, WAKEUP_THREAD_SIGNAL);
    s_watched_signals.for_each_key([&](int signum) {
        sigdelset(&sigset, signum);
    });

    for (;;) {
        int ret = pselect(FD_SETSIZE, &rd_set, &wr_set, &ex_set, nullptr, &sigset);
        if (ret == -1) {
            if (errno == EINTR) {
                if (s_signal_number) {
                    int signo = s_signal_number;
                    s_signal_number = 0;

                    auto* handler = s_watched_signals.get(signo);
                    assert(handler);
                    (*handler)();
                } else if (s_timer_fired_id) {
                    timer_t timer_id = *s_timer_fired_id;
                    s_timer_fired_id = nullptr;

                    int extra_times_expired = timer_getoverrun(timer_id);
                    assert(extra_times_expired >= 0);
                    auto* target = s_timer_objects.get(timer_id);
                    assert(target);
                    EventLoop::queue_event(*target, make_unique<TimerEvent>(1 + extra_times_expired));
                }
                return;
            }
            assert(false);
        }

        if (ret == 0) {
            return;
        }
        break;
    }

    s_selectables.for_each_reverse([&](auto& selectable) {
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
    });
}

void EventLoop::do_event_dispatch() {
    for (;;) {
        pthread_mutex_lock(&m_lock);
        auto events = move(m_events);
        pthread_mutex_unlock(&m_lock);

        if (events.empty()) {
            return;
        }

        for (auto& event : events) {
            if (auto target = event.target.lock()) {
                if (event.event->name() == CallbackEvent::static_event_name()) {
                    static_cast<CallbackEvent&>(*event.event).invoke();
                    continue;
                }

                target->dispatch(*event.event);
            }
        }
    }
}

void EventLoop::setup_signal_handlers() {
    struct sigaction act;
    sigfillset(&act.sa_mask);

    act.sa_flags = SA_SIGINFO;
    act.sa_sigaction = &timer_signal_handler;
    sigaction(TIMER_SIGNAL, &act, nullptr);

    act.sa_sigaction = &wakeup_thread_signal_handler;
    sigaction(WAKEUP_THREAD_SIGNAL, &act, nullptr);

    act.sa_flags = 0;
    act.sa_handler = &private_signal_handler;

    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, TIMER_SIGNAL);
    sigaddset(&set, WAKEUP_THREAD_SIGNAL);
    s_watched_signals.for_each_key([&](int signum) {
        sigaddset(&set, signum);
        sigaction(signum, &act, nullptr);
    });
    sigprocmask(SIG_BLOCK, &set, nullptr);
}

void EventLoop::enter() {
    if (m_should_exit) {
        return;
    }

    m_waiting_thread = pthread_self();
    setup_signal_handlers();

    for (;;) {
        do_event_dispatch();
        if (m_should_exit) {
            return;
        }
        do_select();
    }
}
}
