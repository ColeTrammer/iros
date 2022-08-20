#pragma once

#include <liim/span.h>
#include <liim/utilities.h>

namespace LIIM::Container::Vector {
template<typename T>
concept ReadonlyVectorStorage = requires(T& vector, const T& const_vector) {
    typename T::VectorValue;

    // Basic vector accessors
    { const_vector.capacity() } -> SameAs<size_t>;
    { vector.span() } -> SameAs<Span<typename T::VectorValue>>;
};

template<typename T>
concept VectorStorage = ReadonlyVectorStorage<T> && requires(T& vector, size_t size_value) {
    // Only need 2 mutating operations: one to reserve capacity and one to set the internal size.
    vector.assume_size(size_value);
    vector.reserve(size_value);
};

template<ReadonlyVectorStorage Vec>
using VectorStorageValue = Vec::VectorValue;

template<ReadonlyVectorStorage Vec>
using VectorStorageIterator = VectorStorageValue<Vec>*;

template<ReadonlyVectorStorage Vec>
using VectorStorageConstIterator = VectorStorageValue<Vec> const*;

template<typename Vec, typename T>
concept ReadonlyVectorStorageOf = ReadonlyVectorStorage<Vec> && SameAs<VectorStorageValue<Vec>, T>;

template<typename Vec, typename T>
concept VectorStorageOf = VectorStorage<Vec> && ReadonlyVectorStorageOf<Vec, T>;

template<VectorStorage, typename T>
using VectorStorageResult = T;
}
