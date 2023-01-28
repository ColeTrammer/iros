#pragma once

#include <di/concepts/default_initializable.h>
#include <di/concepts/initializer_list.h>
#include <di/concepts/movable.h>
#include <di/container/concepts/borrowed_container.h>
#include <di/container/concepts/container.h>
#include <di/container/concepts/contiguous_container.h>
#include <di/container/concepts/has_empty_container.h>
#include <di/container/concepts/sized_container.h>
#include <di/container/interface/reconstruct.h>
#include <di/container/meta/container_value.h>
#include <di/container/meta/enable_borrowed_container.h>
#include <di/container/view/view_interface.h>
#include <di/util/move.h>

namespace di::container {
template<concepts::Container Cont>
requires(concepts::Movable<Cont> && !concepts::InitializerList<Cont>)
class OwningView
    : public ViewInterface<OwningView<Cont>>
    , public meta::EnableBorrowedContainer<OwningView<Cont>, concepts::BorrowedContainer<Cont>> {
public:
    constexpr OwningView()
    requires(concepts::DefaultInitializable<Cont>)
    = default;
    constexpr OwningView(OwningView const&) = delete;
    constexpr OwningView(OwningView&&) = default;
    constexpr OwningView(Cont&& container) : m_container(util::move(container)) {}

    constexpr OwningView& operator=(OwningView const&) = delete;
    constexpr OwningView& operator=(OwningView&&) = default;

    constexpr Cont& base() & { return m_container; }
    constexpr Cont const& base() const& { return m_container; }
    constexpr Cont&& base() && { return util::move(m_container); }
    constexpr Cont const&& base() const&& { return util::move(m_container); }

    constexpr auto begin() { return container::begin(m_container); }
    constexpr auto begin() const
    requires(concepts::Container<Cont const>)
    {
        return container::begin(m_container);
    }

    constexpr auto end() { return container::end(m_container); }
    constexpr auto end() const
    requires(concepts::Container<Cont const>)
    {
        return container::end(m_container);
    }

    constexpr bool empty()
    requires(concepts::HasEmptyContainer<Cont>)
    {
        return container::empty(m_container);
    }
    constexpr bool empty() const
    requires(concepts::HasEmptyContainer<Cont const>)
    {
        return container::empty(m_container);
    }

    constexpr auto size()
    requires(concepts::SizedContainer<Cont>)
    {
        return container::size(m_container);
    }
    constexpr auto size() const
    requires(concepts::SizedContainer<Cont const>)
    {
        return container::size(m_container);
    }

    constexpr auto data()
    requires(concepts::ContiguousContainer<Cont>)
    {
        return container::data(m_container);
    }
    constexpr auto data() const
    requires(concepts::ContiguousContainer<Cont const>)
    {
        return container::data(m_container);
    }

private:
    template<typename T, typename U>
    requires(!concepts::SameAs<OwningView, Cont> &&
             concepts::Invocable<decltype(container::reconstruct), InPlaceType<Cont>, T, U>)
    constexpr friend auto tag_invoke(types::Tag<container::reconstruct>, InPlaceType<OwningView>, T&& t, U&& u) {
        return container::reconstruct(in_place_type<Cont>, util::forward<T>(t), util::forward<U>(u));
    }

    Cont m_container;
};
}
