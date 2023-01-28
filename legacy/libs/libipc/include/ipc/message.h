#pragma once

#include <stdint.h>

namespace IPC {

class Stream;

struct Message {
    uint32_t size;
    uint32_t type;
    uint8_t data[];
};

template<typename T>
concept ConcreteMessage = requires(T a, Stream& s) {
    a.serialize(s);
    a.deserialize(s);
    a.serialization_size();
    T::message_type();
};

}
