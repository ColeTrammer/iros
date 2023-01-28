#pragma once

#include <di/container/concepts/prelude.h>
#include <di/container/iterator/iterator_extension.h>
#include <di/container/iterator/sentinel_extension.h>
#include <di/container/meta/enable_borrowed_container.h>
#include <di/container/meta/prelude.h>
#include <di/container/view/view_interface.h>
#include <di/meta/make_unsigned.h>
#include <di/meta/maybe_const.h>
#include <di/util/move.h>

namespace di::container {
template<concepts::InputContainer View>
requires(concepts::View<View>)
class EnumerateView
    : public ViewInterface<EnumerateView<View>>
    , public meta::EnableBorrowedContainer<EnumerateView<View>, concepts::BorrowedContainer<View>> {
private:
    template<bool is_const>
    using Base = meta::MaybeConst<is_const, View>;

    template<bool is_const>
    using SSizeType = meta::ContainerSSizeType<Base<is_const>>;

    template<bool is_const>
    using Iter = meta::ContainerIterator<Base<is_const>>;

    template<bool is_const>
    using Sent = meta::ContainerSentinel<Base<is_const>>;

    template<bool is_const>
    using Value = meta::ContainerValue<Base<is_const>>;

    template<bool is_const>
    using Index = meta::MakeUnsigned<SSizeType<is_const>>;

    template<bool is_const>
    class Iterator
        : public IteratorExtension<Iterator<is_const>, Iter<is_const>, Tuple<Index<is_const>, Value<is_const>>> {
    private:
        using Base = IteratorExtension<Iterator<is_const>, Iter<is_const>, Tuple<Index<is_const>, Value<is_const>>>;

    public:
        Iterator() = default;

        constexpr explicit Iterator(Iter<is_const> base, SSizeType<is_const> index)
            : Base(util::move(base)), m_index(static_cast<Index<is_const>>(index)) {}

        constexpr Iterator(Iterator<!is_const> other)
        requires(is_const && concepts::ConvertibleTo<Iter<false>, Iter<true>>)
            : Base(other.base()), m_index(other.m_index) {}

        constexpr auto operator*() const {
            return Tuple<Index<is_const>, meta::IteratorReference<Iter<is_const>>> { m_index, *this->base() };
        }

        constexpr void advance_one() {
            Base::advance_one();
            ++m_index;
        }

        constexpr void back_one()
        requires(concepts::BidirectionalIterator<Iter<is_const>>)
        {
            Base::back_one();
            --m_index;
        }

        constexpr void advance_n(SSizeType<is_const> n)
        requires(concepts::RandomAccessIterator<Iter<is_const>>)
        {
            Base::advance_n(n);
            m_index += n;
        }

    private:
        template<bool other_is_const>
        friend class Iterator;

        Index<is_const> m_index;
    };

    template<bool is_const>
    class Sentinel : public SentinelExtension<Sentinel<is_const>, Sent<is_const>, Iterator<is_const>, Iter<is_const>> {
    private:
        using Base = SentinelExtension<Sentinel<is_const>, Sent<is_const>, Iterator<is_const>, Iter<is_const>>;

    public:
        Sentinel() = default;

        constexpr explicit Sentinel(Sent<is_const> base) : Base(base) {}

        constexpr Sentinel(Sentinel<!is_const> other)
        requires(is_const && concepts::ConvertibleTo<Sent<false>, Sent<true>>)
            : Base(other.base()) {}
    };

public:
    EnumerateView()
    requires(concepts::DefaultInitializable<View>)
    = default;

    constexpr explicit EnumerateView(View base) : m_base(util::move(base)) {}

    constexpr View base() const&
    requires(concepts::CopyConstructible<View>)
    {
        return m_base;
    }
    constexpr View base() && { return util::move(m_base); }

    constexpr auto begin()
    requires(!concepts::SimpleView<View>)
    {
        return Iterator<false>(container::begin(m_base), 0);
    }

    constexpr auto begin() const
    requires(concepts::Container<View const>)
    {
        return Iterator<true>(container::begin(m_base), 0);
    }

    constexpr auto end()
    requires(!concepts::SimpleView<View>)
    {
        if constexpr (concepts::CommonContainer<View> && concepts::SizedContainer<View>) {
            return Iterator<false>(container::end(m_base), static_cast<SSizeType<false>>(this->size()));
        } else {
            return Sentinel<false>(container::end(m_base));
        }
    }

    constexpr auto end() const
    requires(concepts::Container<View const>)
    {
        if constexpr (concepts::CommonContainer<View const> && concepts::SizedContainer<View const>) {
            return Iterator<false>(container::end(m_base), static_cast<SSizeType<true>>(this->size()));
        } else {
            return Sentinel<false>(container::end(m_base));
        }
    }

    constexpr auto size()
    requires(concepts::SizedContainer<View>)
    {
        return container::size(m_base);
    }

    constexpr auto size() const
    requires(concepts::SizedContainer<View const>)
    {
        return container::size(m_base);
    }

private:
    View m_base;
};

template<typename Con>
EnumerateView(Con&&) -> EnumerateView<meta::AsView<Con>>;
}