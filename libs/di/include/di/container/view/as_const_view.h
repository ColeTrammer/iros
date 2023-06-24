#pragma once

#include <di/container/concepts/prelude.h>
#include <di/container/concepts/simple_view.h>
#include <di/container/interface/cbegin.h>
#include <di/container/interface/cend.h>
#include <di/container/meta/as_view.h>
#include <di/container/meta/enable_borrowed_container.h>
#include <di/container/view/view_interface.h>
#include <di/meta/operations.h>
#include <di/util/move.h>

namespace di::container {
template<concepts::View View>
requires(concepts::InputContainer<View>)
class AsConstView
    : public ViewInterface<AsConstView<View>>
    , public meta::EnableBorrowedContainer<AsConstView<View>, concepts::BorrowedContainer<View>> {
public:
    constexpr AsConstView()
    requires(concepts::DefaultInitializable<View>)
    = default;

    constexpr explicit AsConstView(View view) : m_view(util::move(view)) {}

    constexpr View base() const&
    requires(concepts::CopyConstructible<View>)
    {
        return m_view;
    }

    constexpr View base() && { return util::move(m_view); }

    constexpr auto begin() const
    requires(!concepts::SimpleView<View>)
    {
        return container::cbegin(m_view);
    }

    constexpr auto begin() const
    requires(concepts::Container<View const>)
    {
        return container::cbegin(m_view);
    }

    constexpr auto end() const
    requires(!concepts::SimpleView<View>)
    {
        return container::cend(m_view);
    }

    constexpr auto end() const
    requires(concepts::Container<View const>)
    {
        return container::cend(m_view);
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
AsConstView(Con&&) -> AsConstView<meta::AsView<Con>>;
}
