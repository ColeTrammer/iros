#pragma once

#include <di/container/concepts/prelude.h>
#include <di/container/iterator/next.h>
#include <di/container/meta/enable_borrowed_container.h>
#include <di/container/meta/prelude.h>
#include <di/container/view/view_interface.h>
#include <di/util/move.h>
#include <di/util/non_propagating_cache.h>
#include <di/util/store_if.h>

namespace di::container {
template<concepts::View View>
class DropView
    : public ViewInterface<DropView<View>>
    , public meta::EnableBorrowedContainer<DropView<View>, concepts::BorrowedContainer<View>> {
private:
    constexpr static bool need_cache =
        !concepts::RandomAccessContainer<View const> || !concepts::SizedContainer<View const>;

    using SSizeType = meta::ContainerSSizeType<View>;

public:
    DropView()
    requires(concepts::DefaultInitializable<View>)
    = default;

    constexpr DropView(View base, SSizeType count) : m_base(util::move(base)), m_count(count) {}

    constexpr View base() const&
    requires(concepts::CopyConstructible<View>)
    {
        return m_base;
    }
    constexpr View base() && { return util::move(m_base); }

    constexpr auto begin()
    requires(!concepts::SimpleView<View> || need_cache)
    {
        if constexpr (need_cache) {
            if (m_begin_cache.value.has_value()) {
                return m_begin_cache.value.value();
            }
            return m_begin_cache.value.emplace(
                container::next(container::begin(m_base), m_count, container::end(m_base)));
        } else {
            return container::next(container::begin(m_base), m_count, container::end(m_base));
        }
    }

    constexpr auto begin() const
    requires(!need_cache)
    {
        return container::next(container::begin(m_base), m_count, container::end(m_base));
    }

    constexpr auto end()
    requires(!concepts::SimpleView<View>)
    {
        return container::end(m_base);
    }

    constexpr auto end() const
    requires(concepts::Container<View const>)
    {
        return container::end(m_base);
    }

    constexpr auto size()
    requires(concepts::SizedContainer<View>)
    {
        using SizeType = meta::ContainerSizeType<View>;
        auto size = container::size(m_base);
        auto count = static_cast<SizeType>(m_count);
        return size < count ? 0 : size - count;
    }

    constexpr auto size() const
    requires(concepts::SizedContainer<View const>)
    {
        using SizeType = meta::ContainerSizeType<View const>;
        auto size = container::size(m_base);
        auto count = static_cast<SizeType>(m_count);
        return size < count ? 0 : size - count;
    }

private:
    View m_base;
    SSizeType m_count { 0 };
    [[no_unique_address]] util::StoreIf<util::NonPropagatingCache<meta::ContainerIterator<View>>, need_cache>
        m_begin_cache;
};

template<typename Con>
DropView(Con&&, meta::ContainerSSizeType<Con>) -> DropView<meta::AsView<Con>>;
}