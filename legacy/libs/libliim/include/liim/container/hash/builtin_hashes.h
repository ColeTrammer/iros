#pragma once

#include <liim/container/hash/hashable.h>
#include <liim/container/hash/hasher.h>
#include <liim/forward.h>

namespace LIIM::Container::Hash {
template<typename T>
requires(requires(const T& value, Hasher& hasher) { { value.hash(hasher) }; }) struct HashFunction<T> {
    static constexpr void hash(Hasher& hasher, const T& value) { return value.hash(hasher); }
};

template<typename T>
requires(requires(const T& value, Hasher& hasher) { hasher.add(value); }) struct HashFunction<T> {
    constexpr static void hash(Hasher& hasher, const T& value) { return hasher.add(value); }
};

template<>
struct HashFunction<const char*> {
    constexpr static void hash(Hasher& hasher, const char* pointer) {
        while (*pointer) {
            hasher.add(*pointer++);
        }
    }

    using Matches = Tuple<StringView, String>;
};
}
