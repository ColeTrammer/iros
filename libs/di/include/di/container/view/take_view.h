#pragma once

#include "di/container/algorithm/min.h"
#include "di/container/concepts/prelude.h"
#include "di/container/iterator/counted_iterator.h"
#include "di/container/meta/enable_borrowed_container.h"
#include "di/container/meta/prelude.h"
#include "di/container/view/view_interface.h"
#include "di/meta/util.h"
#include "di/util/move.h"

namespace di::container {
template<concepts::View View>
class TakeView
    : public ViewInterface<TakeView<View>>
    , public meta::EnableBorrowedContainer<TakeView<View>, concepts::BorrowedContainer<View>> {
private:
    template<bool is_const>
    using Base = meta::MaybeConst<is_const, View>;

    template<bool is_const>
    using Iter = meta::ContainerIterator<Base<is_const>>;

    template<bool is_const>
    using Sent = meta::ContainerSentinel<Base<is_const>>;

    template<bool is_const>
    using SSizeType = meta::ContainerSSizeType<Base<is_const>>;

    template<bool is_const>
    class Sentinel {
    public:
        Sentinel() = default;

        constexpr explicit Sentinel(Sent<is_const> sentinel) : m_sentinel(sentinel) {}

        constexpr Sentinel(Sentinel<!is_const> other)
        requires(is_const && concepts::ConvertibleTo<Sent<false>, Sent<true>>)
            : m_sentinel(util::move(other.base())) {}

        constexpr auto base() const { return m_sentinel; }

    private:
        constexpr friend auto operator==(CountedIterator<Iter<is_const>> const& a, Sentinel const& b) -> bool {
            return a.count() == 0 || a.base() == b.base();
        }

        template<bool other_is_const = !is_const>
        requires(concepts::SentinelFor<Sent<is_const>, Iter<other_is_const>>)
        constexpr friend auto operator==(CountedIterator<Iter<other_is_const>> const& a, Sentinel const& b) -> bool {
            return a.count() == 0 || a.base() == b.base();
        }

        Sent<is_const> m_sentinel;
    };

public:
    TakeView()
    requires(concepts::DefaultInitializable<View>)
    = default;

    constexpr TakeView(View base, SSizeType<false> count) : m_base(util::move(base)), m_count(count) {}

    constexpr auto base() const& -> View
    requires(concepts::CopyConstructible<View>)
    {
        return m_base;
    }
    constexpr auto base() && -> View { return util::move(m_base); }

    constexpr auto begin()
    requires(!concepts::SimpleView<View>)
    {
        if constexpr (concepts::SizedContainer<View>) {
            if constexpr (concepts::RandomAccessContainer<View>) {
                return container::begin(m_base);
            } else {
                return CountedIterator(container::begin(m_base), static_cast<SSizeType<false>>(this->size()));
            }
        } else {
            return CountedIterator(container::begin(m_base), m_count);
        }
    }

    constexpr auto begin() const
    requires(concepts::Container<View const>)
    {
        if constexpr (concepts::SizedContainer<View const>) {
            if constexpr (concepts::RandomAccessContainer<View const>) {
                return container::begin(m_base);
            } else {
                return CountedIterator(container::begin(m_base), static_cast<SSizeType<true>>(this->size()));
            }
        } else {
            return CountedIterator(container::begin(m_base), m_count);
        }
    }

    constexpr auto end()
    requires(!concepts::SimpleView<View>)
    {
        if constexpr (concepts::SizedContainer<View>) {
            if constexpr (concepts::RandomAccessContainer<View>) {
                return container::begin(m_base) + static_cast<SSizeType<false>>(this->size());
            } else {
                return default_sentinel;
            }
        } else {
            return Sentinel<false>(container::end(m_base));
        }
    }

    constexpr auto end() const
    requires(concepts::Container<View const>)
    {
        if constexpr (concepts::SizedContainer<View const>) {
            if constexpr (concepts::RandomAccessContainer<View const>) {
                return container::begin(m_base) + static_cast<SSizeType<true>>(this->size());
            } else {
                return default_sentinel;
            }
        } else {
            return Sentinel<true>(container::end(m_base));
        }
    }

    constexpr auto size()
    requires(concepts::SizedContainer<View>)
    {
        using SizeType = decltype(container::size(m_base));
        return container::min(container::size(m_base), static_cast<SizeType>(m_count));
    }

    constexpr auto size() const
    requires(concepts::SizedContainer<View const>)
    {
        using SizeType = decltype(container::size(m_base));
        return container::min(container::size(m_base), static_cast<SizeType>(m_count));
    }

private:
    View m_base;
    SSizeType<false> m_count { 0 };
};

template<typename Con>
TakeView(Con&&, meta::ContainerSSizeType<Con>) -> TakeView<meta::AsView<Con>>;
}
