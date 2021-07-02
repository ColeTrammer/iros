#include <eventloop/event.h>
#include <eventloop/event_loop.h>
#include <eventloop/object.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

class TestObject final : public App::Object {
    APP_OBJECT(TestObject)

public:
    virtual void on_event(const App::Event&) override {
        fprintf(stderr, "Got event!\n");
        exit(0);
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
