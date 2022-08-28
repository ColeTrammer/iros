#pragma once

#include <di/concepts/convertible_to.h>
#include <di/concepts/convertible_to_non_slicing.h>
#include <di/concepts/decay_same_as.h>
#include <di/container/concepts/borrowed_container.h>
#include <di/container/concepts/iterator.h>
#include <di/container/concepts/sentinel_for.h>
#include <di/container/concepts/sized_container.h>
#include <di/container/concepts/sized_sentinel_for.h>
#include <di/container/interface/begin.h>
#include <di/container/interface/end.h>
#include <di/container/interface/size.h>
#include <di/container/iterator/advance.h>
#include <di/container/meta/container_sentinel.h>
#include <di/container/meta/container_size_type.h>
#include <di/container/meta/enable_borrowed_container.h>
#include <di/container/meta/enable_view.h>
#include <di/container/meta/iterator_size_type.h>
#include <di/container/meta/iterator_ssize_type.h>
#include <di/container/view/view_interface.h>
#include <di/util/move.h>
#include <di/util/store_if.h>

namespace di::container {
template<concepts::Iterator Iter, concepts::SentinelFor<Iter> Sent, bool is_sized = concepts::SizedSentinelFor<Sent, Iter>>
requires(is_sized || !concepts::SizedSentinelFor<Sent, Iter>)
class View
    : public ViewInterface<View<Iter, Sent, is_sized>>
    , public meta::EnableBorrowedContainer<View<Iter, Sent, is_sized>> {
private:
    constexpr static bool store_size = is_sized && !concepts::SizedSentinelFor<Sent, Iter>;

    using SizeType = meta::IteratorSizeType<Iter>;
    using SSizeType = meta::IteratorSSizeType<Iter>;

public:
    constexpr View()
    requires(concepts::DefaultInitializable<Iter>)
    = default;

    constexpr View(concepts::ConvertibleToNonSlicing<Iter> auto iterator, Sent sentinel)
    requires(!store_size)
        : m_iterator(util::move(iterator)), m_sentinel(sentinel) {}

    constexpr View(concepts::ConvertibleToNonSlicing<Iter> auto iterator, Sent sentinel, SizeType size)
    requires(is_sized)
        : m_iterator(util::move(iterator)), m_sentinel(sentinel), m_size(size) {}

    template<typename Cont>
    requires(!concepts::DecaySameAs<Cont, View> && concepts::BorrowedContainer<Cont> &&
             concepts::ConvertibleToNonSlicing<meta::ContainerIterator<Cont>, Iter> &&
             concepts::ConvertibleTo<meta::ContainerSentinel<Cont>, Sent>)
    constexpr View(Cont&& container)
    requires(!store_size || concepts::SizedContainer<Cont>)
        : m_iterator(container::begin(container)), m_sentinel(container::end(container)) {
        if constexpr (store_size) {
            m_size.value = static_cast<SizeType>(container::size(container));
        }
    }

    template<concepts::BorrowedContainer Cont>
    requires(concepts::ConvertibleToNonSlicing<meta::ContainerIterator<Cont>, Iter> &&
             concepts::ConvertibleTo<meta::ContainerSentinel<Cont>, Sent>)
    constexpr View(Cont&& container, SizeType size)
    requires(is_sized)
        : View(container::begin(container), container::end(container), size) {}

    constexpr Iter begin() const
    requires(concepts::Copyable<Iter>)
    {
        return m_iterator;
    }

    [[nodiscard]] constexpr Iter begin()
    requires(!concepts::Copyable<Iter>)
    {
        return util::move(m_iterator);
    }

    constexpr Sent end() const { return m_sentinel; }

    constexpr bool empty() const { return m_iterator == m_sentinel; }

    constexpr SizeType size() const
    requires(is_sized)
    {
        if constexpr (store_size) {
            return m_size.value;
        } else {
            return m_sentinel - m_iterator;
        }
    }

    constexpr View& advance(SSizeType n) {
        container::advance(m_iterator, n, m_sentinel);
        return *this;
    }

    [[nodiscard]] constexpr View prev(SSizeType n = 1) const
    requires(concepts::BidirectionalIterator<Iter>)
    {
        auto result = *this;
        result.advance(-n);
        return result;
    }

    [[nodiscard]] constexpr View next(SSizeType n = 1) const&
    requires(concepts::ForwardIterator<Iter>)
    {
        auto result = *this;
        result.advance(n);
        return result;
    }

    [[nodiscard]] constexpr View next(SSizeType n = 1) && {
        this->advance(n);
        return util::move(*this);
    }

private:
    Iter m_iterator;
    Sent m_sentinel;
    [[no_unique_address]] util::StoreIf<SizeType, store_size> m_size;
};

template<concepts::Iterator Iter, concepts::SentinelFor<Iter> Sent>
View(Iter, Sent) -> View<Iter, Sent>;

template<concepts::Iterator Iter, concepts::SentinelFor<Iter> Sent>
View(Iter, Sent, meta::IteratorSizeType<Iter>) -> View<Iter, Sent, true>;

template<concepts::BorrowedContainer Cont>
View(Cont&&)
    -> View<meta::ContainerIterator<Cont>, meta::ContainerSentinel<Cont>,
            concepts::SizedContainer<Cont> || concepts::SizedSentinelFor<meta::ContainerSentinel<Cont>, meta::ContainerIterator<Cont>>>;

template<concepts::BorrowedContainer Cont>
View(Cont&&, meta::ContainerSizeType<Cont>) -> View<meta::ContainerIterator<Cont>, meta::ContainerSentinel<Cont>, true>;
}
