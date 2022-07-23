#pragma once

#include <liim/container/array.h>
#include <liim/utilities.h>

namespace LIIM {
template<typename T, size_t size, size_t alignment>
concept Erasable = sizeof(T) <= size && alignof(T) <= alignment&& TriviallyRelocatable<T>;

template<typename T, size_t size, size_t alignment>
struct ErasedStorage;

template<size_t size, size_t alignment>
struct ErasedStorage<void, size, alignment> {
    alignas(alignment) Array<unsigned char, size> storage;
};

template<typename T, size_t size, size_t alignment>
requires(Erasable<T, size, alignment>) struct ErasedStorage<T, size, alignment> {
    union {
        alignas(alignment) T value;
        alignas(alignment) Array<unsigned char, size> storage;
    };

    constexpr ErasedStorage(T&& value_in) : value(move(value_in)) {}
    constexpr ErasedStorage(Array<unsigned char, size> storage_in) : storage(storage_in) {}
    constexpr ~ErasedStorage() {}

    operator ErasedStorage<void, size, alignment>() const { return ErasedStorage<void, size, alignment>(storage); }
};

template<typename T, size_t size, size_t alignment>
requires(Erasable<T, size, alignment>) ErasedStorage<T, size, alignment> erased_storage_cast(ErasedStorage<void, size, alignment> value) {
    return ErasedStorage<T, size, alignment>(value.storage);
}
}
