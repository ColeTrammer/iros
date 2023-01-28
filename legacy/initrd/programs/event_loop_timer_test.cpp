#include <eventloop/event_loop.h>
#include <eventloop/timer.h>
#include <stdio.h>
#include <stdlib.h>

int main() {
    App::EventLoop loop;

    int count = 0;
    auto timer = App::Timer::create_interval_timer(nullptr, 1000);
    timer->on<App::TimerEvent>({}, [&](auto&) {
        printf("Timer Fired\n");
        if (++count == 10) {
            exit(0);
        }
    });

    loop.enter();
    return 0;
}
