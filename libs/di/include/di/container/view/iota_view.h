#pragma once

#include "di/container/concepts/sized_sentinel_for.h"
#include "di/container/interface/reconstruct.h"
#include "di/container/iterator/iterator_base.h"
#include "di/container/iterator/iterator_category.h"
#include "di/container/iterator/iterator_ssize_type.h"
#include "di/container/iterator/iterator_value.h"
#include "di/container/iterator/sentinel_base.h"
#include "di/container/iterator/unreachable_sentinel.h"
#include "di/container/meta/enable_borrowed_container.h"
#include "di/container/meta/iterator_ssize_type.h"
#include "di/container/types/bidirectional_iterator_tag.h"
#include "di/container/types/forward_iterator_tag.h"
#include "di/container/types/input_iterator_tag.h"
#include "di/container/types/random_access_iterator_tag.h"
#include "di/container/view/view_interface.h"
#include "di/math/to_unsigned.h"
#include "di/meta/compare.h"
#include "di/meta/core.h"
#include "di/meta/language.h"
#include "di/meta/operations.h"
#include "di/meta/util.h"

namespace di::container {
namespace detail {
    template<typename T>
    concept IotaIncrementable = requires(T i) {
        { ++i } -> concepts::SameAs<T&>;
        { i++ } -> concepts::SameAs<T>;
    };

    template<typename T>
    concept IotaDecrementable = IotaIncrementable<T> && requires(T i) {
        { --i } -> concepts::SameAs<T&>;
        { i-- } -> concepts::SameAs<T>;
    };

    template<typename T>
    concept IotaAdvancable = IotaDecrementable<T> && concepts::TotallyOrdered<T> &&
                             requires(T i, T const ci, meta::IteratorSSizeType<T> const n) {
                                 { i += n } -> concepts::SameAs<T&>;
                                 { i -= n } -> concepts::SameAs<T&>;
                                 T(ci + n);
                                 T(n + ci);
                                 T(ci - n);
                                 T(n - ci);
                                 { ci - ci } -> concepts::ConvertibleTo<meta::IteratorSSizeType<T>>;
                             };
}

template<concepts::Copyable T, concepts::Semiregular Bound = UnreachableSentinel>
requires(concepts::detail::WeaklyEqualityComparableWith<T, Bound> &&
         requires(T& value) {
             typename meta::IteratorSSizeType<T>;
             ++value;
         })
class IotaView
    : public ViewInterface<IotaView<T, Bound>>
    , public meta::EnableBorrowedContainer<IotaView<T, Bound>> {
private:
    using SSizeType = meta::IteratorSSizeType<T>;

    constexpr static bool is_bounded = !concepts::SameAs<Bound, UnreachableSentinel>;

    class Sentinel;

    class Iterator
        : public IteratorBase<
              Iterator,
              meta::Conditional<detail::IotaAdvancable<T>, RandomAccessIteratorTag,
                                meta::Conditional<detail::IotaDecrementable<T>, BidirectionalIteratorTag,
                                                  meta::Conditional<detail::IotaIncrementable<T>, ForwardIteratorTag,
                                                                    InputIteratorTag>>>,
              T, SSizeType> {
    public:
        Iterator()
        requires(concepts::DefaultInitializable<T>)
        = default;

        constexpr explicit Iterator(T value) : m_value(value) {}

        Iterator(Iterator const&) = default;
        Iterator(Iterator&&) = default;

        auto operator=(Iterator const&) -> Iterator& = default;
        auto operator=(Iterator&&) -> Iterator& = default;

        Iterator(Iterator const&)
        requires(!detail::IotaIncrementable<T>)
        = delete;

        auto operator=(Iterator const&) -> Iterator& requires(!detail::IotaIncrementable<T>) = delete;

        constexpr auto operator*() const -> T { return m_value; }

        constexpr void advance_one() { ++m_value; }

        constexpr void back_one()
        requires(detail::IotaDecrementable<T>)
        {
            --m_value;
        }

        constexpr void advance_n(SSizeType n)
        requires(detail::IotaAdvancable<T>)
        {
            if constexpr (concepts::UnsignedInteger<T>) {
                if (n >= 0) {
                    m_value += static_cast<T>(n);
                } else {
                    m_value -= static_cast<T>(-n);
                }
            } else {
                m_value += n;
            }
        }

    private:
        friend class IotaView;
        friend class Sentinel;

        constexpr friend auto operator==(Iterator const& a, Iterator const& b) -> bool
        requires(concepts::EqualityComparable<T>)
        {
            return a.m_value == b.m_value;
        }

        constexpr friend auto operator<=>(Iterator const& a, Iterator const& b)
        requires(concepts::ThreeWayComparable<T>)
        {
            return a.m_value <=> b.m_value;
        }

        constexpr friend auto operator-(Iterator const& a, Iterator const& b) -> SSizeType
        requires(detail::IotaAdvancable<T>)
        {
            if constexpr (concepts::SignedInteger<T>) {
                return static_cast<SSizeType>(static_cast<SSizeType>(a.m_value) - static_cast<SSizeType>(b.m_value));
            } else if constexpr (concepts::UnsignedInteger<T>) {
                return b.m_value > a.m_value ? static_cast<SSizeType>(-static_cast<SSizeType>(b.m_value - a.m_value))
                                             : static_cast<SSizeType>(a.m_value - b.m_value);
            } else {
                return a.m_value - b.m_value;
            }
        }

    public:
        T m_value;
    };

    class Sentinel : public SentinelBase<Sentinel> {
    public:
        constexpr Sentinel() = default;
        constexpr explicit Sentinel(Bound bound) : m_bound(bound) {}

        constexpr auto difference(Iterator const& a) const -> SSizeType { return -(a.m_value - this->m_bound); }

    private:
        friend class IotaView;

        constexpr friend auto operator==(Iterator const& a, Sentinel const& b) -> bool {
            return a.m_value == b.m_bound;
        }

        Bound m_bound;
    };

public:
    constexpr IotaView()
    requires(concepts::DefaultInitializable<T>)
    = default;

    constexpr explicit IotaView(T value) : m_value(value) {}

    constexpr IotaView(meta::TypeIdentity<T> value, meta::TypeIdentity<Bound> bound) : m_value(value), m_bound(bound) {}

    constexpr IotaView(Iterator first, Iterator last)
    requires(concepts::SameAs<T, Bound>)
        : m_value(first.m_value), m_bound(last.m_value) {}

    constexpr IotaView(Iterator first, Sentinel last)
    requires(!concepts::SameAs<T, Bound> && is_bounded)
        : m_value(first.m_value), m_bound(last.m_bound) {}

    constexpr IotaView(Iterator first, UnreachableSentinel)
    requires(!is_bounded)
        : m_value(first.m_value) {}

    constexpr auto begin() const -> Iterator { return Iterator(m_value); }

    constexpr auto end() const {
        if constexpr (is_bounded) {
            return Sentinel(m_bound);
        } else {
            return unreachable_sentinel;
        }
    }

    constexpr auto end() const -> Iterator
    requires(concepts::SameAs<T, Bound>)
    {
        return Iterator(m_bound);
    }

    constexpr auto size() const
    requires((concepts::SameAs<T, Bound> && detail::IotaAdvancable<T>) ||
             (concepts::Integer<T> && concepts::Integer<Bound>) || (concepts::SizedSentinelFor<Bound, T>) )
    {
        if constexpr (concepts::Integer<T> && concepts::Integer<Bound>) {
            if (m_value < 0) {
                return ((m_bound < 0) ? math::to_unsigned(-m_value) - math::to_unsigned(-m_value)
                                      : math::to_unsigned(m_bound) + math::to_unsigned(-m_value));
            }
            return math::to_unsigned(m_bound) - math::to_unsigned(m_value);
        } else {
            return math::to_unsigned(m_bound - m_value);
        }
    }

private:
    template<concepts::OneOf<Iterator, meta::Conditional<is_bounded, Sentinel, UnreachableSentinel>> Sent>
    constexpr friend auto tag_invoke(types::Tag<container::reconstruct>, Iterator first, Sent last) {
        if constexpr (concepts::SameAs<Iterator, Sent>) {
            return IotaView<decltype(*util::move(first)), decltype(*util::move(last))>(*util::move(first),
                                                                                       *util::move(last));
        } else {
            return IotaView(util::move(first), util::move(last));
        }
    }

    T m_value {};
    Bound m_bound {};
};

template<typename T, typename Bound>
requires(!concepts::Integer<T> || !concepts::Integer<Bound> ||
         concepts::SignedInteger<T> == concepts::SignedInteger<Bound>)
IotaView(T, Bound) -> IotaView<T, Bound>;
}
