#include <eventloop/event.h>
#include <eventloop/event_loop.h>
#include <eventloop/object.h>
#include <eventloop/widget_events.h>
#include <liim/format.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

class TestObject final : public App::Object {
    APP_OBJECT(TestObject)

public:
    virtual void initialize() {
        on<App::ThemeChangeEvent>([](const App::ThemeChangeEvent&) {
            error_log("Got event!");
            exit(0);
        });

        Object::initialize();
    }

private:
};

static SharedPtr<App::Object> s_object;

int main() {
    App::EventLoop loop;
    s_object = TestObject::create(nullptr);

    pthread_t tid;
    pthread_create(
        &tid, nullptr,
        [](void*) -> void* {
            sleep(1);
            App::EventLoop::queue_event(s_object, make_unique<App::ThemeChangeEvent>());
            return nullptr;
        },
        nullptr);

    loop.enter();
    return 0;
}
