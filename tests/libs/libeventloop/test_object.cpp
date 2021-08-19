#include <eventloop/event.h>
#include <eventloop/object.h>
#include <test/test.h>

using namespace App;

struct CustomEvent : Event {
    APP_EVENT(CustomEvent)

public:
    CustomEvent() : Event(static_event_name()) {}
};

struct CustomEvent2 : Event {
    APP_EVENT(CustomEvent2)

public:
    CustomEvent2() : Event(static_event_name()) {}
};

TEST(object, basic_events) {
    auto object = Object::create(nullptr);

    int count = 0;
    object->on<CustomEvent>([&](const auto&) {
        count++;
    });

    EXPECT(!object->dispatch(CustomEvent {}));
    EXPECT(!object->dispatch(CustomEvent {}));
    EXPECT(!object->dispatch(CustomEvent {}));

    EXPECT(!object->dispatch(CustomEvent2 {}));

    EXPECT_EQ(count, 3);
}
