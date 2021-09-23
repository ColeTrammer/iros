#include <liim/function.h>
#include <pthread.h>
#include <thread/background_job.h>

namespace Thread {
struct Closure {
    Function<void()> task;
};

static void* start_thread(void* closure_void) {
    auto* closure = static_cast<Closure*>(closure_void);
    closure->task();
    delete closure;
    return nullptr;
}

void BackgroundJob::start(Function<void()> task) {
    pthread_attr_t attr;
    pthread_attr_init(&attr);

    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    pthread_t id;
    pthread_create(&id, &attr, &start_thread, static_cast<void*>(new Closure { move(task) }));

    pthread_attr_destroy(&attr);
}
}
