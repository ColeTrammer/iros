#pragma once

#include "di/container/concepts/borrowed_container.h"
#include "di/container/concepts/iterator.h"
#include "di/container/concepts/sentinel_for.h"
#include "di/container/concepts/sized_container.h"
#include "di/container/concepts/sized_sentinel_for.h"
#include "di/container/interface/begin.h"
#include "di/container/interface/end.h"
#include "di/container/interface/size.h"
#include "di/container/iterator/advance.h"
#include "di/container/meta/container_sentinel.h"
#include "di/container/meta/container_size_type.h"
#include "di/container/meta/enable_borrowed_container.h"
#include "di/container/meta/enable_view.h"
#include "di/container/meta/iterator_size_type.h"
#include "di/container/meta/iterator_ssize_type.h"
#include "di/container/view/view_interface.h"
#include "di/meta/constexpr.h"
#include "di/meta/operations.h"
#include "di/meta/util.h"
#include "di/util/add_member_get.h"
#include "di/util/forward_like.h"
#include "di/util/move.h"
#include "di/util/store_if.h"
#include "di/vocab/tuple/enable_generate_structed_bindings.h"

namespace di::container {
template<concepts::Iterator Iter, concepts::SentinelFor<Iter> Sent = Iter,
         bool is_sized = concepts::SizedSentinelFor<Sent, Iter>>
requires(is_sized || !concepts::SizedSentinelFor<Sent, Iter>)
class View
    : public ViewInterface<View<Iter, Sent, is_sized>>
    , public meta::EnableBorrowedContainer<View<Iter, Sent, is_sized>>
    , public util::AddMemberGet<View<Iter, Sent, is_sized>> {
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
        : m_iterator(util::move(iterator)), m_sentinel(di::move(sentinel)) {}

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

    constexpr auto begin() const -> Iter
    requires(concepts::Copyable<Iter>)
    {
        return m_iterator;
    }

    [[nodiscard]] constexpr auto begin() -> Iter
    requires(!concepts::Copyable<Iter>)
    {
        return util::move(m_iterator);
    }

    constexpr auto end() const -> Sent { return m_sentinel; }

    constexpr auto empty() const -> bool { return m_iterator == m_sentinel; }

    constexpr auto size() const -> SizeType
    requires(is_sized)
    {
        if constexpr (store_size) {
            return m_size.value;
        } else {
            return m_sentinel - m_iterator;
        }
    }

    constexpr auto advance(SSizeType n) -> View& {
        container::advance(m_iterator, n, m_sentinel);
        return *this;
    }

    [[nodiscard]] constexpr auto prev(SSizeType n = 1) const -> View
    requires(concepts::BidirectionalIterator<Iter>)
    {
        auto result = *this;
        result.advance(-n);
        return result;
    }

    [[nodiscard]] constexpr auto next(SSizeType n = 1) const& -> View
    requires(concepts::ForwardIterator<Iter>)
    {
        auto result = *this;
        result.advance(n);
        return result;
    }

    [[nodiscard]] constexpr auto next(SSizeType n = 1) && -> View {
        this->advance(n);
        return util::move(*this);
    }

private:
    constexpr friend auto tag_invoke(types::Tag<vocab::tuple_element>, types::InPlaceType<View>, Constexpr<0ZU>)
        -> InPlaceType<Iter> {}
    constexpr friend auto tag_invoke(types::Tag<vocab::tuple_element>, types::InPlaceType<View>, Constexpr<1ZU>)
        -> InPlaceType<Sent> {}

    constexpr friend auto tag_invoke(types::Tag<vocab::tuple_element>, types::InPlaceType<View const>, Constexpr<0ZU>)
        -> InPlaceType<Iter const> {}
    constexpr friend auto tag_invoke(types::Tag<vocab::tuple_element>, types::InPlaceType<View const>, Constexpr<1ZU>)
        -> InPlaceType<Sent const> {}

    constexpr friend auto tag_invoke(types::Tag<vocab::tuple_size>, types::InPlaceType<View>) -> types::size_t {
        return 2;
    }

    template<concepts::DecaySameAs<View> Self>
    constexpr friend auto tag_invoke(types::Tag<util::get_in_place>, Constexpr<0ZU>, Self&& self)
        -> meta::Like<Self, Iter> {
        return util::forward_like<Self>(self.m_iterator);
    }

    template<concepts::DecaySameAs<View> Self>
    constexpr friend auto tag_invoke(types::Tag<util::get_in_place>, Constexpr<1ZU>, Self&& self)
        -> meta::Like<Self, Sent> {
        return util::forward_like<Self>(self.m_sentinel);
    }

    template<concepts::DecaySameAs<View> Self, typename T>
    requires(!concepts::SameAs<Iter, Sent> && (concepts::SameAs<T, Iter> || concepts::SameAs<T, Sent>) )
    constexpr friend auto tag_invoke(types::Tag<util::get_in_place>, types::InPlaceType<T>, Self&& self)
        -> decltype(auto) {
        if constexpr (concepts::SameAs<T, Iter>) {
            return util::get<0>(util::forward<Self>(self));
        } else {
            return util::get<1>(util::forward<Self>(self));
        }
    }

    Iter m_iterator;
    Sent m_sentinel;
    [[no_unique_address]] util::StoreIf<SizeType, store_size> m_size;
};

template<concepts::Iterator Iter, concepts::SentinelFor<Iter> Sent>
View(Iter, Sent) -> View<Iter, Sent>;

template<concepts::Iterator Iter, concepts::SentinelFor<Iter> Sent>
View(Iter, Sent, meta::IteratorSizeType<Iter>) -> View<Iter, Sent, true>;

template<concepts::BorrowedContainer Cont>
View(Cont&&) -> View<meta::ContainerIterator<Cont>, meta::ContainerSentinel<Cont>,
                     concepts::SizedContainer<Cont> ||
                         concepts::SizedSentinelFor<meta::ContainerSentinel<Cont>, meta::ContainerIterator<Cont>>>;

template<concepts::BorrowedContainer Cont>
View(Cont&&, meta::ContainerSizeType<Cont>) -> View<meta::ContainerIterator<Cont>, meta::ContainerSentinel<Cont>, true>;
}

namespace di {
using container::View;
}
