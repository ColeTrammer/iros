#include <app/event_loop.h>
#include <app/timer.h>
#include <stdio.h>
#include <stdlib.h>

int main() {
    App::EventLoop loop;

    int count = 0;
    auto timer = App::Timer::create_interval_timer(
        nullptr,
        [&](int) {
            printf("Timer Fired\n");
            if (++count == 10) {
                exit(0);
            }
        },
        1000);

    loop.enter();
    return 0;
}
