#pragma once

#include <liim/hash/forward.h>
#include <liim/utilities.h>

namespace LIIM::Hash {
template<typename T>
struct HashFunction {};

template<typename T>
requires(requires(const T& value, Hasher& hasher) {
    { value.hash(hasher) } -> SameAs<uint64_t>;
}) struct HashFunction<T> {
    static constexpr void hash(Hasher& hasher, const T& value) { return value.hash(hasher); }
};

template<typename T>
using HashForType = HashFunction<typename RemoveCVRef<T>::type>;

template<typename T>
concept Hashable = requires(const T& value, Hasher& hasher) {
    HashForType<T>::hash(hasher, value);
};
}
