#pragma once

#include <di/concepts/convertible_to.h>
#include <di/concepts/copyable.h>
#include <di/concepts/default_initializable.h>
#include <di/concepts/equality_comparable.h>
#include <di/concepts/integer.h>
#include <di/concepts/same_as.h>
#include <di/concepts/semiregular.h>
#include <di/concepts/signed_integer.h>
#include <di/concepts/three_way_comparable.h>
#include <di/concepts/totally_ordered.h>
#include <di/concepts/unsigned_integer.h>
#include <di/concepts/weakly_equality_comparable_with.h>
#include <di/container/concepts/sized_sentinel_for.h>
#include <di/container/iterator/iterator_category.h>
#include <di/container/iterator/iterator_ssize_type.h>
#include <di/container/iterator/iterator_value.h>
#include <di/container/iterator/unreachable_sentinel.h>
#include <di/container/meta/enable_borrowed_container.h>
#include <di/container/meta/iterator_ssize_type.h>
#include <di/container/types/bidirectional_iterator_tag.h>
#include <di/container/types/forward_iterator_tag.h>
#include <di/container/types/input_iterator_tag.h>
#include <di/container/types/random_access_iterator_tag.h>
#include <di/container/view/view_interface.h>
#include <di/math/to_unsigned.h>
#include <di/meta/type_identity.h>

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
    concept IotaAdvancable =
        IotaDecrementable<T> && concepts::TotallyOrdered<T> && requires(T i, T const ci, meta::IteratorSSizeType<T> const n) {
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
         requires(T & value) {
             typename meta::IteratorSSizeType<T>;
             ++value;
         })
class IotaView
    : public ViewInterface<IotaView<T, Bound>>
    , public meta::EnableBorrowedContainer<IotaView<T, Bound>> {
private:
    using SSizeType = meta::IteratorSSizeType<T>;

    constexpr static bool is_bounded = concepts::SameAs<Bound, UnreachableSentinel>;

    class Sentinel;

    class Iterator {
    public:
        constexpr Iterator()
        requires(concepts::DefaultInitializable<T>)
        = default;

        constexpr explicit Iterator(T value) : m_value(value) {}

        constexpr T operator*() const { return m_value; }

        constexpr Iterator& operator++() {
            ++m_value;
            return *this;
        }

        constexpr void operator++(int) { ++m_value; }

        constexpr Iterator operator++(int)
        requires(detail::IotaIncrementable<T>)
        {
            auto temp = *this;
            ++m_value;
            return temp;
        }

        constexpr Iterator& operator--()
        requires(detail::IotaDecrementable<T>)
        {
            --m_value;
            return *this;
        }

        constexpr Iterator operator--(int)
        requires(detail::IotaDecrementable<T>)
        {
            auto temp = *this;
            --m_value;
            return temp;
        }

        constexpr Iterator& operator+=(SSizeType n)
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
            return *this;
        }

        constexpr Iterator& operator-=(SSizeType n)
        requires(detail::IotaAdvancable<T>)
        {
            if constexpr (concepts::UnsignedInteger<T>) {
                if (n >= 0) {
                    m_value -= static_cast<T>(n);
                } else {
                    m_value += static_cast<T>(-n);
                }
            } else {
                m_value -= n;
            }
            return *this;
        }

        constexpr T operator[](SSizeType n) const
        requires(detail::IotaAdvancable<T>)
        {
            return T(m_value + n);
        }

    private:
        friend class IotaView;
        friend class Sentinal;

        constexpr friend bool operator==(Iterator const& a, Iterator const& b)
        requires(concepts::EqualityComparable<T>)
        {
            return a.m_value == b.m_value;
        }

        constexpr friend auto operator<=>(Iterator const& a, Iterator const& b)
        requires(concepts::ThreeWayComparable<T>)
        {
            return a.m_value <=> b.m_value;
        }

        constexpr friend Iterator operator+(Iterator x, SSizeType n)
        requires(detail::IotaAdvancable<T>)
        {
            x += n;
            return x;
        }

        constexpr friend Iterator operator+(SSizeType n, Iterator x)
        requires(detail::IotaAdvancable<T>)
        {
            x += n;
            return x;
        }

        constexpr friend Iterator operator-(Iterator x, SSizeType n)
        requires(detail::IotaAdvancable<T>)
        {
            x -= n;
            return x;
        }

        constexpr friend SSizeType operator-(Iterator const& a, Iterator const& b)
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

        constexpr friend auto tag_invoke(types::Tag<iterator_category>, types::InPlaceType<Iterator>) {
            if constexpr (detail::IotaAdvancable<T>) {
                return types::RandomAccessIteratorTag {};
            } else if constexpr (detail::IotaDecrementable<T>) {
                return types::BidirectionalIteratorTag {};
            } else if constexpr (detail::IotaIncrementable<T>) {
                return types::ForwardIteratorTag {};
            } else {
                return types::InputIteratorTag {};
            }
        }

        constexpr friend T tag_invoke(types::Tag<iterator_value>, types::InPlaceType<Iterator>) {}
        constexpr friend T tag_invoke(types::Tag<iterator_reference>, types::InPlaceType<Iterator>) {}
        constexpr friend SSizeType tag_invoke(types::Tag<iterator_ssize_type>, types::InPlaceType<Iterator>) {}

        T m_value;
    };

    class Sentinel {
    public:
        constexpr Sentinel() = default;
        constexpr explicit Sentinel(Bound bound) : m_bound(bound) {}

    private:
        friend class IotaView;

        constexpr friend bool operator==(Iterator const& a, Sentinel const& b) { return a.m_value == b.m_bound; }

        constexpr friend SSizeType operator-(Iterator const& a, Sentinel const& b)
        requires(concepts::SizedSentinelFor<Bound, T>)
        {
            return a.m_value - b.m_bound;
        }

        constexpr friend SSizeType operator-(Sentinel const& a, Iterator const& b) { return -(b.m_value - a.m_bound); }

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

    constexpr Iterator begin() const { return Iterator(m_value); }

    constexpr auto end() const {
        if constexpr (is_bounded) {
            return Sentinel(m_bound);
        } else {
            return unreachable_sentinel;
        }
    }

    constexpr Iterator end() const
    requires(concepts::SameAs<T, Bound>)
    {
        return Iterator(m_bound);
    }

    constexpr auto size() const
    requires((concepts::SameAs<T, Bound> && detail::IotaAdvancable<T>) || (concepts::Integer<T> && concepts::Integer<Bound>) ||
             (concepts::SizedSentinelFor<Bound, T>) )
    {
        if constexpr (concepts::Integer<T> && concepts::Integer<Bound>) {
            return (m_value < 0) ? ((m_bound < 0) ? math::to_unsigned(-m_value) - math::to_unsigned(-m_value)
                                                  : math::to_unsigned(m_bound) + math::to_unsigned(-m_value))
                                 : math::to_unsigned(m_bound) - math::to_unsigned(m_value);
        } else {
            return math::to_unsigned(m_bound - m_value);
        }
    }

private:
    T m_value {};
    Bound m_bound {};
};

template<typename T, typename Bound>
requires(!concepts::Integer<T> || !concepts::Integer<Bound> || concepts::SignedInteger<T> == concepts::SignedInteger<Bound>)
IotaView(T, Bound) -> IotaView<T, Bound>;
}
