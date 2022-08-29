#pragma once

#include <di/concepts/decay_same_as.h>
#include <di/concepts/object.h>
#include <di/container/concepts/contiguous_container.h>
#include <di/container/concepts/has_empty_container.h>
#include <di/container/concepts/sized_container.h>
#include <di/container/interface/empty.h>
#include <di/container/meta/enable_borrowed_container.h>
#include <di/container/view/view_interface.h>
#include <di/util/declval.h>
#include <di/util/forward.h>
#include <di/util/reference_wrapper.h>

namespace di::container {
template<concepts::Container Cont>
requires(concepts::Object<Cont>)
class RefView
    : public ViewInterface<RefView<Cont>>
    , public meta::EnableBorrowedContainer<RefView<Cont>> {
private:
    template<typename X>
    constexpr static void delete_rvalues(X&);

    template<typename X>
    constexpr static void delete_rvalues(X&&) = delete;

public:
    template<typename T>
    requires(!concepts::DecaySameAs<T, RefView> && requires { delete_rvalues(util::declval<T>()); })
    constexpr RefView(T&& container) : m_container(static_cast<Cont&>(util::forward<T>())) {}

    constexpr Cont& base() const { return m_container.get(); }

    constexpr auto begin() const { return container::begin(m_container.get()); }
    constexpr auto end() const { return container::end(m_container.get()); }

    constexpr bool empty() const
    requires(concepts::HasEmptyContainer<Cont>)
    {
        return container::empty(m_container.get());
    }

    constexpr auto size() const
    requires(concepts::SizedContainer<Cont>)
    {
        return container::size(m_container.get());
    }

    constexpr auto data() const
    requires(concepts::ContiguousContainer<Cont>)
    {
        return container::data(m_container.get());
    }

private:
    util::ReferenceWrapper<Cont> m_container;
};

template<typename Cont>
RefView(Cont&) -> RefView<Cont>;
}
