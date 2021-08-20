#include <eventloop/event.h>
#include <eventloop/object.h>
#include <test/test.h>

using namespace App;

class CustomEvent : public Event {
    APP_EVENT(CustomEvent)

public:
    CustomEvent() : Event(static_event_name()) {}
};

class CustomEvent2 : public Event {
    APP_EVENT(CustomEvent2)

public:
    CustomEvent2() : Event(static_event_name()) {}
};

class ConsumableEvent : public Event {
    APP_EVENT_REQUIRES_HANDLING(ConsumableEvent)

public:
    ConsumableEvent() : Event(static_event_name()) {}
};

TEST(object, events_basic) {
    auto object = Object::create(nullptr);

    int count = 0;
    object->on<CustomEvent>({}, [&](const auto&) {
        count++;
    });

    EXPECT(!object->dispatch(CustomEvent {}));
    EXPECT(!object->dispatch(CustomEvent {}));
    EXPECT(!object->dispatch(CustomEvent {}));

    EXPECT(!object->dispatch(CustomEvent2 {}));

    EXPECT_EQ(count, 3);
}

TEST(object, events_deregistration) {
    auto object = Object::create(nullptr);
    auto listener = Object::create(nullptr);

    int count = 0;
    object->on<CustomEvent>(*listener, [&](const auto&) {
        count++;
    });

    EXPECT(!object->dispatch(CustomEvent {}));
    EXPECT(!object->dispatch(CustomEvent {}));

    listener = nullptr;

    EXPECT(!object->dispatch(CustomEvent {}));

    EXPECT_EQ(count, 2);
}

TEST(object, event_consuming) {
    auto object = Object::create(nullptr);

    int count = 0;
    object->on<CustomEvent>({}, [&](const auto&) {
        count++;
    });
    object->on<CustomEvent>({}, [&](const auto&) {
        count++;
    });
    object->on<CustomEvent>({}, [&](const auto&) {
        count++;
    });

    object->on<ConsumableEvent>({}, [&](const auto&) {
        count++;
        return false;
    });
    object->on<ConsumableEvent>({}, [&](const auto&) {
        count++;
        return true;
    });
    object->on<ConsumableEvent>({}, [&](const auto&) {
        count++;
        return true;
    });

    EXPECT(!object->dispatch(CustomEvent {}));
    EXPECT_EQ(count, 3);

    EXPECT(object->dispatch(ConsumableEvent {}));
    EXPECT_EQ(count, 5);
}

TEST(object, event_multiple) {
    auto object = Object::create(nullptr);

    int count = 0;

    object->on<CustomEvent, CustomEvent2>({}, [&](auto&& event) {
        using Type = LIIM::decay_t<decltype(event)>;

        if constexpr (LIIM::IsSame<CustomEvent, Type>::value) {
            EXPECT_EQ(event.name(), CustomEvent::static_event_name());
            count++;
        } else {
            EXPECT_EQ(event.name(), CustomEvent2::static_event_name());
            count += 2;
        }
    });

    EXPECT(!object->dispatch(CustomEvent {}));
    EXPECT(!object->dispatch(CustomEvent2 {}));

    EXPECT_EQ(count, 3);
}
