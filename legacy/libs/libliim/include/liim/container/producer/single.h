#pragma once

#include <liim/container/producer/repeat.h>

namespace LIIM::Container::Producer {
template<typename T>
constexpr Repeat<T> single(T value) {
    return Repeat(1, move(value));
}
}

using LIIM::Container::Producer::single;
