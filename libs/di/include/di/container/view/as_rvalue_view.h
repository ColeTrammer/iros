#pragma once

#include "di/container/concepts/prelude.h"
#include "di/container/concepts/simple_view.h"
#include "di/container/iterator/move_iterator.h"
#include "di/container/meta/as_view.h"
#include "di/container/meta/enable_borrowed_container.h"
#include "di/container/view/view_interface.h"
#include "di/meta/operations.h"
#include "di/util/move.h"

namespace di::container {
template<concepts::View View>
requires(concepts::InputContainer<View>)
class AsRValueView
    : public ViewInterface<AsRValueView<View>>
    , public meta::EnableBorrowedContainer<AsRValueView<View>, concepts::BorrowedContainer<View>> {
public:
    constexpr AsRValueView()
    requires(concepts::DefaultInitializable<View>)
    = default;

    constexpr explicit AsRValueView(View view) : m_view(util::move(view)) {}

    constexpr auto base() const& -> View
    requires(concepts::CopyConstructible<View>)
    {
        return m_view;
    }

    constexpr auto base() && -> View { return util::move(m_view); }

    constexpr auto begin()
    requires(!concepts::SimpleView<View>)
    {
        return MoveIterator(container::begin(m_view));
    }

    constexpr auto begin() const
    requires(concepts::Container<View const>)
    {
        return MoveIterator(container::begin(m_view));
    }

    constexpr auto end()
    requires(!concepts::SimpleView<View>)
    {
        if constexpr (concepts::CommonContainer<View>) {
            return MoveIterator(container::end(m_view));
        } else {
            return MoveSentinel(container::end(m_view));
        }
    }

    constexpr auto end() const
    requires(concepts::Container<View const>)
    {
        if constexpr (concepts::CommonContainer<View const>) {
            return MoveIterator(container::end(m_view));
        } else {
            return MoveSentinel(container::end(m_view));
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
AsRValueView(Con&&) -> AsRValueView<meta::AsView<Con>>;
}
