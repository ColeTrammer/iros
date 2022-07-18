#pragma once

#include <liim/error/forward.h>
#include <liim/utilities.h>

namespace LIIM::Error {
template<typename T>
concept Erasable = sizeof(T) <= sizeof(void*) && alignof(T) <= sizeof(void*) && TriviallyRelocatable<T>;

template<>
struct ErrorTransport<void> {
    void* value;
};

template<Erasable T>
struct ErrorTransport<T> {
    union {
        T value;
        void* storage;
    };

    constexpr ErrorTransport(T&& value_in) : value(move(value_in)) {}
    constexpr ErrorTransport(void* storage_in) : storage(storage_in) {}
    constexpr ~ErrorTransport() {}

    operator ErrorTransport<>() const { return reinterpret_cast<ErrorTransport<>>(*this); }
};

template<Erasable T>
ErrorTransport<T> error_transport_cast(ErrorTransport<> value) {
    return ErrorTransport<T>(value.value);
}
}
