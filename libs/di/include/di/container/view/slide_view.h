#pragma once

#include <di/container/concepts/prelude.h>
#include <di/container/iterator/iterator_base.h>
#include <di/container/iterator/sentinel_base.h>
#include <di/container/meta/prelude.h>
#include <di/container/types/prelude.h>
#include <di/container/view/view_interface.h>
#include <di/util/non_propagating_cache.h>
#include <di/util/store_if.h>

namespace di::container {
namespace detail {
    template<typename T>
    concept SlideCachesNothing = concepts::RandomAccessContainer<T> && concepts::SizedContainer<T>;

    template<typename T>
    concept SlideCachesLast =
        (!SlideCachesNothing<T>) &&concepts::BidirectionalContainer<T> && concepts::CommonContainer<T>;

    template<typename T>
    concept SlideCachesFirst = (!SlideCachesNothing<T>);
}

template<concepts::ForwardContainer View>
requires(concepts::View<View>)
class SlideView
    : public ViewInterface<SlideView<View>>
    , public meta::EnableBorrowedContainer<SlideView<View>, concepts::BorrowedContainer<View>> {
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
    using ValueType = decltype(container::reconstruct(
        in_place_type<Base<is_const>>, util::declval<Iter<is_const> const&>(), util::declval<Iter<is_const> const&>()));

    class Sentinel;

    template<bool is_const>
    class Iterator
        : public IteratorBase<Iterator<is_const>,
                              meta::Conditional<concepts::RandomAccessIterator<Iter<is_const>>, RandomAccessIteratorTag,
                                                meta::Conditional<concepts::BidirectionalIterator<Iter<is_const>>,
                                                                  BidirectionalIteratorTag, ForwardIteratorTag>>,
                              ValueType<is_const>, SSizeType<is_const>> {
    private:
        constexpr Iterator(Iter<is_const> current, SSizeType<is_const> window_size)
        requires(!detail::SlideCachesFirst<Base<is_const>>)
            : m_current(util::move(current)), m_window_size(window_size) {}

        constexpr Iterator(Iter<is_const> current, Iter<is_const> last_element, SSizeType<is_const> window_size)
        requires(detail::SlideCachesFirst<Base<is_const>>)
            : m_current(util::move(current)), m_last_element(util::move(last_element)), m_window_size(window_size) {}

    public:
        Iterator() = default;

        constexpr Iterator(Iterator<!is_const> other)
        requires(is_const && concepts::ConvertibleTo<Iter<false>, Iter<true>>)
            : m_current(util::move(other.m_current)), m_window_size(other.m_window_size) {}

        constexpr auto operator*() const {
            if constexpr (detail::SlideCachesFirst<Base<is_const>>) {
                return container::reconstruct(in_place_type<Base<is_const>>, m_current,
                                              container::next(m_last_element.value));
            } else {
                return container::reconstruct(in_place_type<Base<is_const>>, m_current, m_current + m_window_size);
            }
        }

        constexpr void advance_one() {
            if constexpr (detail::SlideCachesFirst<Base<is_const>>) {
                ++m_last_element.value;
            }
            ++m_current;
        }

        constexpr void back_one()
        requires(concepts::BidirectionalIterator<Iter<is_const>>)
        {
            if constexpr (detail::SlideCachesFirst<Base<is_const>>) {
                --m_last_element.value;
            }
            --m_current;
        }

        constexpr void advance_n(SSizeType<is_const> n)
        requires(concepts::RandomAccessIterator<Iter<is_const>>)
        {
            if constexpr (detail::SlideCachesFirst<Base<is_const>>) {
                m_last_element.value += n;
            }
            m_current += n;
        }

    private:
        template<bool>
        friend class Iterator;

        friend class Sentinel;

        friend class SlideView;

        constexpr friend bool operator==(Iterator const& a, Iterator const& b) {
            if constexpr (detail::SlideCachesFirst<Base<is_const>>) {
                return a.m_last_element.value == b.m_last_element.value;
            } else {
                return a.m_current == b.m_current;
            }
        }

        constexpr friend auto operator<=>(Iterator const& a, Iterator const& b)
        requires(concepts::ThreeWayComparable<Iter<is_const>>)
        {
            return a.m_current <=> b.m_current;
        }

        constexpr friend auto operator-(Iterator const& a, Iterator const& b)
        requires(concepts::SizedSentinelFor<Iter<is_const>, Iter<is_const>>)
        {
            if constexpr (detail::SlideCachesFirst<Base<is_const>>) {
                return a.m_last_element.value - b.m_last_element.value;
            } else {
                return a.m_current - b.m_current;
            }
        }

        Iter<is_const> m_current {};
        [[no_unique_address]] util::StoreIf<Iter<is_const>, detail::SlideCachesFirst<Base<is_const>>> m_last_element {};
        SSizeType<is_const> m_window_size { 0 };
    };

    class Sentinel : public SentinelBase<Sentinel> {
    private:
        constexpr explicit Sentinel(Sent<false> base) : m_base(base) {}

    public:
        Sentinel() = default;

        constexpr bool equals(Iterator<false> const& other) { return m_base == other.m_last_element; }

        constexpr auto difference(Iterator<false> const& other)
        requires(concepts::SizedSentinelFor<Sent<false>, Iter<false>>)
        {
            return m_base - other.m_last_element;
        }

    private:
        friend class SlideView;

        Sent<false> m_base;
    };

public:
    SlideView()
    requires(concepts::DefaultInitializable<View>)
    = default;

    constexpr explicit SlideView(View base, SSizeType<false> window_size)
        : m_base(util::move(base)), m_window_size(window_size) {
        DI_ASSERT_GT(window_size, 0);
    }

    constexpr View base() const&
    requires(concepts::CopyConstructible<View>)
    {
        return m_base;
    }
    constexpr View base() && { return util::move(m_base); }

    constexpr auto window_size() const { return m_window_size; }

    constexpr auto begin()
    requires(!concepts::SimpleView<View> || !detail::SlideCachesNothing<View const>)
    {
        if constexpr (detail::SlideCachesFirst<View>) {
            if (!m_begin_cache.value) {
                m_begin_cache.value.emplace(Iterator<false>(
                    container::begin(m_base),
                    container::next(container::begin(m_base), m_window_size - 1, container::end(m_base)),
                    m_window_size));
            }
            return m_begin_cache.value.value();
        } else {
            return Iterator<false>(container::begin(m_base), m_window_size);
        }
    }

    constexpr auto begin() const
    requires(detail::SlideCachesNothing<View const>)
    {
        return Iterator<true>(container::begin(m_base), m_window_size);
    }

    constexpr auto end()
    requires(!concepts::SimpleView<View> || !detail::SlideCachesNothing<View const>)
    {
        if constexpr (detail::SlideCachesNothing<View>) {
            return Iterator<false>(container::begin(m_base) + SSizeType<false>(size()), m_window_size);
        } else if constexpr (detail::SlideCachesLast<View>) {
            if (!m_end_cache.value) {
                m_end_cache.value.emplace(Iterator<false>(
                    container::prev(container::end(m_base), m_window_size - 1, container::begin(m_base)),
                    container::end(m_base), m_window_size));
            }
            return m_end_cache.value.value();
        } else if constexpr (concepts::CommonContainer<View>) {
            return Iterator<false>(container::end(m_base), container::end(m_base), m_window_size);
        } else {
            return Sentinel(container::end(m_base));
        }
    }

    constexpr auto end() const
    requires(detail::SlideCachesNothing<View const>)
    {
        return begin() + SSizeType<true>(size());
    }

    constexpr auto size()
    requires(concepts::SizedContainer<View>)
    {
        auto size = container::distance(m_base) - m_window_size + 1;
        if (size < 0) {
            size = 0;
        }
        return math::to_unsigned(size);
    }

    constexpr auto size() const
    requires(concepts::SizedContainer<View const>)
    {
        auto size = container::distance(m_base) - m_window_size + 1;
        if (size < 0) {
            size = 0;
        }
        return math::to_unsigned(size);
    }

private:
    View m_base;
    SSizeType<false> m_window_size {};
    [[no_unique_address]] util::StoreIf<util::NonPropagatingCache<Iterator<false>>, !detail::SlideCachesNothing<View>>
        m_end_cache;
    [[no_unique_address]] util::StoreIf<util::NonPropagatingCache<Iterator<false>>, !detail::SlideCachesNothing<View>>
        m_begin_cache;
};

template<typename Con>
SlideView(Con&&, meta::ContainerSSizeType<Con>) -> SlideView<meta::AsView<Con>>;
}
