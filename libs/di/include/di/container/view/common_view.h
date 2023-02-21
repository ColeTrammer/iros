#pragma once

#include <di/container/iterator/common_iterator.h>
#include <di/container/view/view_interface.h>

namespace di::container {
template<concepts::View View>
requires(!concepts::CommonContainer<View> && concepts::Copyable<meta::ContainerIterator<View>>)
class CommonView
    : public ViewInterface<CommonView<View>>
    , public meta::EnableBorrowedContainer<CommonView<View>, concepts::BorrowedContainer<View>> {
public:
    constexpr CommonView()
    requires(concepts::DefaultInitializable<View>)
    = default;

    constexpr explicit CommonView(View view) : m_view(util::move(view)) {}

    constexpr View base() const&
    requires(concepts::CopyConstructible<View>)
    {
        return m_view;
    }

    constexpr View base() && { return util::move(m_view); }

    constexpr auto begin() const
    requires(!concepts::SimpleView<View>)
    {
        if constexpr (concepts::RandomAccessContainer<View> && concepts::SizedContainer<View>) {
            return container::begin(m_view);
        } else {
            return CommonIterator<meta::ContainerIterator<View>, meta::ContainerSentinel<View>>(
                container::begin(m_view));
        }
    }

    constexpr auto begin() const
    requires(concepts::Container<View const>)
    {
        if constexpr (concepts::RandomAccessContainer<View const> && concepts::SizedContainer<View const>) {
            return container::begin(m_view);
        } else {
            return CommonIterator<meta::ContainerIterator<View const>, meta::ContainerSentinel<View const>>(
                container::begin(m_view));
        }
    }

    constexpr auto end() const
    requires(!concepts::SimpleView<View>)
    {
        if constexpr (concepts::RandomAccessContainer<View> && concepts::SizedContainer<View>) {
            return container::begin(m_view) + container::size(m_view);
        } else {
            return CommonIterator<meta::ContainerIterator<View>, meta::ContainerSentinel<View>>(container::end(m_view));
        }
    }

    constexpr auto end() const
    requires(concepts::Container<View const>)
    {
        if constexpr (concepts::RandomAccessContainer<View const> && concepts::SizedContainer<View const>) {
            return container::begin(m_view) + container::size(m_view);
        } else {
            return CommonIterator<meta::ContainerIterator<View const>, meta::ContainerSentinel<View const>>(
                container::end(m_view));
        }
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
    View m_view;
};

template<typename Con>
CommonView(Con&&) -> CommonView<meta::AsView<Con>>;
}
