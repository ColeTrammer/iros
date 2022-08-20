#pragma once

#include <liim/construct.h>
#include <liim/container/algorithm/rotate.h>
#include <liim/container/algorithm/size.h>
#include <liim/container/concepts.h>
#include <liim/container/producer/iterator_container.h>
#include <liim/container/vector/vector_storage.h>
#include <liim/option.h>

namespace LIIM::Container::Vector::Algorithm {
namespace Detail {
    template<typename T>
    constexpr T relocate(T& lvalue) {
        auto result = move(lvalue);
        lvalue.~T();
        return result;
    }

    template<typename T>
    constexpr void relocate_range_right(T* data, T* end, size_t count) {
        while (--end != data) {
            construct_at(&end[count], relocate(*end));
        }
    }

    template<typename T>
    constexpr void relocate_range_left(T* begin, T* mid, T* end) {
        while (mid != end) {
            *begin++ = relocate(*mid++);
        }
    }
}

template<ReadonlyVectorStorage Vec>
constexpr auto data(Vec const& vector) {
    return vector.span().data();
}

template<ReadonlyVectorStorage Vec>
constexpr auto data(Vec& vector) {
    return vector.span().data();
}

template<ReadonlyVectorStorage Vec>
constexpr size_t size(Vec const& vector) {
    return vector.span().size();
}

template<ReadonlyVectorStorage Vec>
constexpr bool empty(Vec const& vector) {
    return Algorithm::size(vector) == 0;
}

template<ReadonlyVectorStorage Vec>
constexpr VectorStorageIterator<Vec> begin(Vec& vector) {
    return Algorithm::data(vector);
}

template<ReadonlyVectorStorage Vec>
constexpr VectorStorageIterator<Vec> begin(Vec const& vector) {
    return Algorithm::data(vector);
}

template<ReadonlyVectorStorage Vec>
constexpr VectorStorageIterator<Vec> end(Vec& vector) {
    return Algorithm::data(vector) + Algorithm::size(vector);
}

template<ReadonlyVectorStorage Vec>
constexpr VectorStorageIterator<Vec> end(Vec const& vector) {
    return Algorithm::data(vector) + Algorithm::size(vector);
}

template<ReadonlyVectorStorage Vec>
constexpr VectorStorageIterator<Vec> iterator(Vec& vector, size_t index) {
    return Algorithm::begin(vector) + index;
}

template<ReadonlyVectorStorage Vec>
constexpr VectorStorageIterator<Vec> iterator(Vec const& vector, size_t index) {
    return Algorithm::begin(vector) + index;
}

template<ReadonlyVectorStorage Vec>
constexpr size_t iterator_index(Vec& vector, VectorStorageConstIterator<Vec> position) {
    return position - Algorithm::begin(vector);
}

template<VectorStorage Vec>
constexpr void clear(Vec& vector) {
    // This could be made for expressive, and handle trivial types.
    for (auto& object : iterator_container(Algorithm::begin(vector), Algorithm::end(vector))) {
        using T = VectorStorageValue<Vec>;
        object.~T();
    }

    vector.assume_size(0);

    // This could call shrink_to_fit()?
}

template<VectorStorage Vec>
constexpr VectorStorageResult<Vec, void> push_back(Vec& vector, VectorStorageValue<Vec>&& value) {
    auto new_size = Algorithm::size(vector) + 1;
    vector.reserve(new_size);

    construct_at(&Algorithm::data(vector)[new_size - 1], move(value));
    vector.assume_size(new_size);
}

template<VectorStorage Vec>
constexpr VectorStorageResult<Vec, void> push_back(Vec& vector, VectorStorageValue<Vec> const& value) {
    return Algorithm::push_back(vector, auto(value));
}

template<VectorStorage Vec, Container C>
requires(ConstructibleFrom<VectorStorageValue<Vec>, ContainerValueType<C>>) constexpr VectorStorageResult<Vec, void> append_container(
    Vec& vector, C&& container) {
    if constexpr (SizedContainer<C>) {
        vector.reserve(Algorithm::size(vector) + Alg::size(container));
    }

    for (auto&& value : container) {
        Algorithm::push_back(vector, VectorStorageValue<Vec>(static_cast<decltype(value)&&>(value)));
    }
}

template<VectorStorage Vec>
constexpr VectorStorageResult<Vec, void> insert(Vec& vector, VectorStorageConstIterator<Vec> position, VectorStorageValue<Vec>&& value) {
    auto new_size = Algorithm::size(vector) + 1;
    vector.reverse(new_size);

    Detail::relocate_range_right(const_cast<VectorStorageValue<Vec>*>(position), Algorithm::end(vector), 1);

    construct_at(position, move(value));

    vector.assume_size(new_size);
}

template<VectorStorage Vec>
constexpr VectorStorageResult<Vec, void> insert(Vec& vector, VectorStorageConstIterator<Vec> position,
                                                VectorStorageValue<Vec> const& value) {
    Algorithm::insert(vector, position, auto(value));
}

template<VectorStorage Vec, Container C>
constexpr VectorStorageResult<Vec, void> insert_container(Vec& vector, VectorStorageConstIterator<Vec> position, C&& container) {
    auto position_index = Algorithm::iterator_index(vector, position);
    // if constexpr (SizedContainer<C>) {
    //     auto size_to_insert = Alg::size(container);
    //     auto new_size = Algorithm::size(vector) + size_to_insert;
    //     vector.reserve(new_size);

    //     Detail::relocate_range_right(const_cast<VectorStorageValue<Vec>*>(position), Algorithm::end(vector), size_to_insert);

    //     position = Algorithm::iterator(vector, position_index);
    //     for (auto&& value : container) {
    //         construct_at(position++, static_cast<decltype(value)&&>(value));
    //     }

    //     vector.assume_size(new_size);
    // } else {
    auto old_size = Algorithm::size(vector);
    for (auto&& value : container) {
        Algorithm::push_back(vector, static_cast<decltype(value)&&>(value));
    }
    Alg::rotate(iterator_container(Algorithm::iterator(vector, position_index), Algorithm::end(vector)),
                Algorithm::iterator(vector, old_size));
    // }
}

template<VectorStorage Vec>
constexpr auto pop_back(Vec& vector) -> Option<VectorStorageValue<Vec>> {
    if (empty(vector)) {
        return None {};
    }

    auto new_size = Algorithm::size(vector) - 1;
    auto result = Detail::relocate(Algorithm::data(vector)[new_size]);
    vector.assume_size(new_size);
    return result;
}

template<VectorStorage Vec>
constexpr void erase(Vec& vector, VectorStorageConstIterator<Vec> begin, VectorStorageConstIterator<Vec> end) {
    auto count_to_delete = end - begin;
    Detail::relocate_range_left(const_cast<VectorStorageValue<Vec>*>(begin), const_cast<VectorStorageValue<Vec>*>(end),
                                Algorithm::end(vector));
    vector.assume_size(Algorithm::size(vector) - count_to_delete);
}

template<VectorStorage Vec>
constexpr void erase(Vec& vector, VectorStorageConstIterator<Vec> position) {
    assert(position != Algorithm::end(vector));
    return Algorithm::erase(vector, position, position + 1);
}

template<VectorStorage Vec>
constexpr void erase_unstable(Vec& vector, VectorStorageConstIterator<Vec> begin, VectorStorageConstIterator<Vec> end) {
    return Algorithm::erase(vector, begin, end);
}

template<VectorStorage Vec>
constexpr void erase_unstable(Vec& vector, VectorStorageConstIterator<Vec> position) {
    assert(position != Algorithm::end(vector));
    return Algorithm::erase_unstable(vector, position, position + 1);
}

template<VectorStorage Vec, Container C>
requires(ConstructibleFrom<VectorStorageValue<Vec>, ContainerValueType<C>>) constexpr void replace(Vec& vector,
                                                                                                   VectorStorageConstIterator<Vec> begin,
                                                                                                   VectorStorageConstIterator<Vec> end,
                                                                                                   C&& container) {
    Algorithm::erase(vector, begin, end);
    return Algorithm::insert_container(vector, begin, forward<C>(container));
}
}
