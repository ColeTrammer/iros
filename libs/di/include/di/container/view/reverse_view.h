#pragma once

#include <di/container/concepts/bidirectional_container.h>
#include <di/container/concepts/borrowed_container.h>
#include <di/container/concepts/common_container.h>
#include <di/container/concepts/view.h>
#include <di/container/interface/begin.h>
#include <di/container/interface/end.h>
#include <di/container/interface/size.h>
#include <di/container/iterator/next.h>
#include <di/container/iterator/reverse_iterator.h>
#include <di/container/meta/as_view.h>
#include <di/container/meta/container_iterator.h>
#include <di/container/meta/enable_borrowed_container.h>
#include <di/container/view/view_interface.h>
#include <di/meta/operations.h>
#include <di/util/move.h>
#include <di/util/non_propagating_cache.h>
#include <di/util/store_if.h>

namespace di::container {
template<concepts::View View>
requires(concepts::BidirectionalContainer<View>)
class ReverseView
    : public ViewInterface<ReverseView<View>>
    , public meta::EnableBorrowedContainer<ReverseView<View>, concepts::BorrowedContainer<View>> {
private:
    using Iter = meta::ContainerIterator<View>;

public:
    constexpr ReverseView()
    requires(concepts::DefaultInitializable<View>)
    = default;

    constexpr ReverseView(View view) : m_view(util::move(view)) {}

    constexpr auto base() const& -> View
    requires(concepts::CopyConstructible<View>)
    {
        return m_view;
    }

    constexpr auto base() && -> View
    requires(concepts::CopyConstructible<View>)
    {
        return util::move(m_view);
    }

    constexpr auto begin() -> ReverseIterator<Iter> {
        if (m_begin_cache.value.has_value()) {
            return *m_begin_cache.value;
        } else {
            return m_begin_cache.value.emplace(
                container::make_reverse_iterator(container::next(container::begin(m_view), container::end(m_view))));
        }
    }

    constexpr auto begin() -> ReverseIterator<Iter>
    requires(concepts::CommonContainer<View>)
    {
        return container::make_reverse_iterator(container::end(m_view));
    }

    constexpr auto begin() const
    requires(concepts::CommonContainer<View const>)
    {
        return container::make_reverse_iterator(container::end(m_view));
    }

    constexpr auto end() -> ReverseIterator<Iter> { return container::make_reverse_iterator(container::begin(m_view)); }

    constexpr auto end() const
    requires(concepts::CommonContainer<View const>)
    {
        return container::make_reverse_iterator(container::begin(m_view));
    }

    constexpr auto size()
    requires(concepts::SizedContainer<View>)
    {
        return container::size(m_view);
    }

    constexpr auto size() const
    requires(concepts::SizedContainer<View const>)
    {
        return container::size(m_view);
    }

private:
    View m_view {};
    [[no_unique_address]] util::StoreIf<util::NonPropagatingCache<ReverseIterator<meta::ContainerIterator<View>>>,
                                        !concepts::CommonContainer<View>> m_begin_cache;
};

template<typename Con>
ReverseView(Con&&) -> ReverseView<meta::AsView<Con>>;
}
