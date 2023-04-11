#pragma once

#include <di/container/action/sequence.h>
#include <di/container/algorithm/destroy.h>
#include <di/container/algorithm/rotate.h>
#include <di/container/algorithm/uninitialized_relocate.h>
#include <di/container/algorithm/uninitialized_relocate_backwards.h>
#include <di/container/ring/mutable_ring.h>
#include <di/container/ring/ring_iterator.h>
#include <di/container/vector/vector_resize.h>
#include <di/util/create.h>
#include <di/vocab/expected/prelude.h>

namespace di::container::ring {
constexpr auto size(concepts::detail::ConstantRing auto& ring) {
    return ring.span().size();
}

constexpr auto size_bytes(concepts::detail::ConstantRing auto& ring) {
    return ring.span().size_bytes();
}

constexpr bool empty(concepts::detail::ConstantRing auto& ring) {
    return ring::size(ring) == 0;
}

constexpr auto* begin_pointer(concepts::detail::ConstantRing auto& ring) {
    return ring.span().data();
}

constexpr auto* end_pointer(concepts::detail::ConstantRing auto& ring) {
    return ring::begin_pointer(ring) + ring.capacity();
}

constexpr auto* head_pointer(concepts::detail::ConstantRing auto& ring) {
    return ring::begin_pointer(ring) + ring.head();
}

constexpr auto* tail_pointer(concepts::detail::ConstantRing auto& ring) {
    return ring::begin_pointer(ring) + ring.tail();
}

template<concepts::detail::ConstantRing Ring, typename Value = meta::detail::RingValue<Ring>>
constexpr auto begin(Ring& ring) {
    return RingIterator<Value>(ring::head_pointer(ring), ring::head_pointer(ring), ring::tail_pointer(ring),
                               ring::begin_pointer(ring), ring::end_pointer(ring), ring::empty(ring));
}

template<concepts::detail::ConstantRing Ring, typename Value = meta::detail::RingValue<Ring>>
constexpr auto end(Ring& ring) {
    return RingIterator<Value>(ring::tail_pointer(ring), ring::head_pointer(ring), ring::tail_pointer(ring),
                               ring::begin_pointer(ring), ring::end_pointer(ring), true);
}

constexpr decltype(auto) lookup(concepts::detail::ConstantRing auto& ring, usize index) {
    return ring::begin(ring)[index];
}

constexpr auto at(concepts::detail::ConstantRing auto& ring, usize index) {
    return lift_bool(index < ring::size(ring)) % [&] {
        return util::ref(ring::lookup(ring, index));
    };
}

constexpr auto front(concepts::detail::ConstantRing auto& ring) {
    return ring::at(ring, 0);
}

constexpr auto back(concepts::detail::ConstantRing auto& ring) {
    // NOTE: This is safe because unsigned underflow is defined behavior. If the ring is empty, then at() will return
    // nullopt.
    return ring::at(ring, ring::size(ring) - 1u);
}

template<concepts::detail::MutableRing Ring, typename Value = meta::detail::RingValue<Ring>>
requires(!concepts::Const<Ring>)
constexpr auto iterator(Ring&, RingIterator<Value const> iterator) {
    // SAFETY: this is safe because we are provided a mutable reference to the ring.
    return iterator.unconst_unsafe();
}

template<concepts::detail::ConstantRing Ring>
requires(!concepts::Const<Ring>)
constexpr auto iterator(Ring& ring, usize index) {
    DI_ASSERT_LT_EQ(index, ring::size(ring));
    return ring::begin(ring) + index;
}

template<concepts::detail::ConstantRing Ring>
constexpr auto iterator(Ring const& ring, usize index) {
    DI_ASSERT_LT_EQ(index, ring::size(ring));
    return ring::begin(ring) + index;
}

constexpr void clear(concepts::detail::MutableRing auto& ring) {
    container::destroy(ring::begin(ring), ring::end(ring));
    ring.assume_size(0);
    ring.assume_head(0);
    ring.assume_tail(0);
}

template<concepts::detail::MutableRing Ring, typename R = meta::detail::RingAllocResult<Ring>>
constexpr R reserve(Ring& ring, usize capacity) {
    if (capacity <= ring.capacity()) {
        return util::create<R>();
    }

    auto size = ring::size(ring);
    auto temp = Ring();
    return invoke_as_fallible([&] {
               return temp.reserve_from_nothing(capacity);
           }) % [&] {
        auto new_buffer = ring::begin_pointer(temp);
        // FIXME: Use an optimized algorithm which breaks the ring buffer into 2 contigous parts.
        container::uninitialized_relocate(ring::begin(ring), ring::end(ring), new_buffer, new_buffer + capacity);
        temp.assume_size(size);
        temp.assume_head(0);
        temp.assume_tail(size);
        ring.assume_size(0);
        ring.assume_head(0);
        ring.assume_tail(0);
        util::swap(ring, temp);
    } | try_infallible;
}

template<concepts::detail::MutableRing Ring, typename... Args>
requires(concepts::ConstructibleFrom<meta::detail::RingValue<Ring>, Args...>)
constexpr decltype(auto) emplace_back(Ring& ring, Args&&... args) {
    auto size = ring::size(ring);
    return invoke_as_fallible([&] {
               return ring::reserve(ring, size + 1);
           }) % [&] {
        auto end = ring::tail_pointer(ring);
        auto result = util::construct_at(end, util::forward<Args>(args)...);
        ring.assume_size(size + 1);
        ring.assume_tail((ring.tail() + 1) % ring.capacity());
        return util::ref(*result);
    } | try_infallible;
}

template<concepts::detail::MutableRing Ring, concepts::InputContainer Con, typename T = meta::detail::RingValue<Ring>,
         typename R = meta::detail::RingAllocResult<Ring>>
requires(concepts::ContainerCompatible<Con, T>)
constexpr R append_container(Ring& ring, Con&& container) {
    return container::sequence(util::forward<Con>(container), [&]<typename X>(X&& value) {
        return as_fallible(ring::emplace_back(ring, util::forward<X>(value)));
    });
}

constexpr auto pop_back(concepts::detail::MutableRing auto& ring) {
    auto size = ring::size(ring);
    return lift_bool(size > 0) % [&] {
        auto new_size = size - 1;
        auto result = util::relocate(ring::lookup(ring, new_size));
        ring.assume_size(new_size);
        ring.assume_tail((ring.tail() + ring.capacity() - 1) % ring.capacity());
        return result;
    };
}

template<concepts::detail::MutableRing Ring, typename... Args>
requires(concepts::ConstructibleFrom<meta::detail::RingValue<Ring>, Args...>)
constexpr decltype(auto) emplace_front(Ring& ring, Args&&... args) {
    auto size = ring::size(ring);
    return invoke_as_fallible([&] {
               return ring::reserve(ring, size + 1);
           }) % [&] {
        auto new_head = (ring.head() + ring.capacity() - 1) % ring.capacity();
        ring.assume_head(new_head);

        auto head = ring::head_pointer(ring);
        auto result = util::construct_at(head, util::forward<Args>(args)...);

        ring.assume_size(size + 1);
        return util::ref(*result);
    } | try_infallible;
}

template<concepts::detail::MutableRing Ring, concepts::InputContainer Con, typename T = meta::detail::RingValue<Ring>,
         typename R = meta::detail::RingAllocResult<Ring>>
requires(concepts::ContainerCompatible<Con, T>)
constexpr R prepend_container(Ring& ring, Con&& container) {
    return container::sequence(util::forward<Con>(container), [&]<typename X>(X&& value) {
        return as_fallible(ring::emplace_front(ring, util::forward<X>(value)));
    });
}

constexpr auto pop_front(concepts::detail::MutableRing auto& ring) {
    auto size = ring::size(ring);
    return lift_bool(size > 0) % [&] {
        auto new_size = size - 1;
        auto result = util::relocate(ring::lookup(ring, 0));
        ring.assume_size(new_size);
        ring.assume_head((ring.head() + 1) % ring.capacity());
        return result;
    };
}

template<concepts::detail::MutableRing Ring, typename T = meta::detail::RingValue<Ring>, typename... Args>
requires(concepts::ConstructibleFrom<meta::detail::RingValue<Ring>, Args...>)
constexpr auto emplace(Ring& ring, RingIterator<T const> it, Args&&... args) {
    auto position = it - ring::begin(ring);
    return as_fallible(ring::emplace_back(ring, util::forward<Args>(args)...)) % [&](auto&) {
        auto new_it = ring::begin(ring) + position;
        auto last = ring::end(ring);
        container::rotate(new_it, last - 1, last);
        return new_it;
    } | try_infallible;
}

template<concepts::detail::MutableRing Ring, typename Value = meta::detail::RingValue<Ring>>
constexpr auto erase(Ring& ring, RingIterator<Value const> first, RingIterator<Value const> last) {
    if (first == last) {
        return ring::iterator(ring, first);
    }

    // Rotate the range [first, last) to the end of the buffer.
    auto const first_position = first - ring::begin(ring);
    auto const to_remove = last - first;
    auto [new_tail, end] = container::rotate(ring::iterator(ring, first), ring::iterator(ring, last), ring::end(ring));

    // Destroy the trailing elements.
    container::destroy(new_tail, end);

    ring.assume_size(ring::size(ring) - to_remove);
    ring.assume_tail((ring.tail() + ring.capacity() - to_remove) % ring.capacity());
    return ring::iterator(ring, first_position);
}

template<concepts::detail::MutableRing Ring, typename Value = meta::detail::RingValue<Ring>>
constexpr auto erase(Ring& ring, RingIterator<Value const> citerator) {
    return ring::erase(ring, citerator, citerator + 1);
}

constexpr auto make_contigous(concepts::detail::MutableRing auto& ring) {
    auto head = ring.head();
    auto tail = ring.tail();
    auto size = ring::size(ring);
    if (head < tail) {
        // Move the range [head, tail) to the beginning of the buffer.
        container::uninitialized_relocate_backwards(ring::head_pointer(ring) + head, ring::head_pointer(ring) + tail,
                                                    ring::head_pointer(ring), ring::end_pointer(ring));
        ring.assume_head(0);
        ring.assume_tail(size);
        return ring.span();
    }

    // Move the range [head, capacity) to directly after tail.
    container::uninitialized_relocate(ring::head_pointer(ring) + head, ring::end_pointer(ring),
                                      ring::head_pointer(ring) + tail, ring::end_pointer(ring));
    // Rotate the range [0, tail) into place.
    container::rotate(ring::head_pointer(ring), ring::head_pointer(ring) + tail, ring::head_pointer(ring) + size);
    ring.assume_head(0);
    ring.assume_tail(size);
    return ring.span();
}

template<concepts::detail::MutableRing Ring, typename T = meta::detail::RingValue<Ring>>
requires(concepts::DefaultConstructible<T>)
constexpr auto resize(Ring& ring, usize count) {
    ring::make_contigous(ring);
    return invoke_as_fallible([&] {
               return vector::resize(ring, count);
           }) % [&] {
        ring.assume_tail(ring::size(ring));
    } | try_infallible;
}

template<concepts::detail::MutableRing Ring, typename T = meta::detail::RingValue<Ring>>
requires(concepts::CopyConstructible<T>)
constexpr auto resize(Ring& ring, usize count, T const& value) {
    ring::make_contigous(ring);
    return invoke_as_fallible([&] {
               return vector::resize(ring, count, value);
           }) % [&] {
        ring.assume_tail(ring::size(ring));
    } | try_infallible;
}
}
