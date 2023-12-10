#pragma once

#include <di/container/concepts/prelude.h>
#include <di/container/iterator/distance.h>
#include <di/container/iterator/next.h>
#include <di/container/iterator/reverse_iterator.h>
#include <di/container/iterator/unreachable_sentinel.h>
#include <di/container/meta/prelude.h>
#include <di/container/view/view_interface.h>
#include <di/meta/util.h>
#include <di/util/non_propagating_cache.h>
#include <di/util/store_if.h>

namespace di::container {
template<concepts::ForwardContainer View>
requires(concepts::View<View>)
class CycleView : public ViewInterface<CycleView<View>> {
private:
    template<bool is_const>
    using Base = meta::MaybeConst<is_const, View>;

    template<bool is_const>
    using Iter = meta::ContainerIterator<Base<is_const>>;

    template<bool is_const>
    using Value = meta::ContainerValue<Base<is_const>>;

    template<bool is_const>
    using SSizeType = meta::ContainerSSizeType<Base<is_const>>;

    template<bool is_const>
    using Parent = meta::MaybeConst<is_const, CycleView>;

    template<bool is_const>
    class Iterator
        : public IteratorBase<Iterator<is_const>,
                              meta::Conditional<concepts::RandomAccessIterator<Iter<is_const>>, RandomAccessIteratorTag,
                                                meta::Conditional<concepts::BidirectionalIterator<Iter<is_const>>,
                                                                  BidirectionalIteratorTag, ForwardIteratorTag>>,
                              Value<is_const>, SSizeType<is_const>> {
    private:
        friend class CycleView;

        constexpr explicit Iterator(Parent<is_const>* parent, Iter<is_const> base)
            : m_parent(parent), m_base(util::move(base)) {}

        constexpr auto get_end() {
            if constexpr (concepts::CommonContainer<Base<is_const>>) {
                return container::end(m_parent->base_ref());
            } else {
                if (!m_parent->m_end_cache) {
                    m_parent->m_end_cache.value.emplace(container::next(m_base, container::end(m_parent->base_ref())));
                }
                return m_parent->m_end_cache.value.value();
            }
        }

    public:
        Iterator() = default;

        template<bool other_is_const = !is_const>
        constexpr Iterator(Iterator<other_is_const> other)
        requires(is_const && concepts::ConvertibleTo<Iter<false>, Iter<true>>)
            : m_parent(other.m_parent), m_base(util::move(other.m_base)), m_cycle_number(other.m_cycle_number) {}

        constexpr auto base() const& { return m_base; }
        constexpr auto base() && { return util::move(m_base); }

        constexpr decltype(auto) operator*() const { return *m_base; }

        constexpr void advance_one() {
            if (++m_base == get_end()) {
                ++m_cycle_number;
                m_base = container::begin(m_parent->base_ref());
            }
        }

        constexpr void back_one()
        requires(concepts::BidirectionalIterator<Iter<is_const>>)
        {
            if (m_base == container::begin(m_parent->base_ref())) {
                --m_cycle_number;
                m_base = get_end();
            }
            --m_base;
        }

        constexpr void advance_n(SSizeType<is_const> n)
        requires(concepts::RandomAccessIterator<Iter<is_const>>)
        {
            auto start = container::begin(m_parent->base_ref());
            auto size = container::distance(start, get_end());

            auto offset = m_base - start;
            auto new_offset = (offset + n) % size;

            if (new_offset < 0) {
                new_offset += size;
            }
            m_base = start + new_offset;
            m_cycle_number += (offset + n) / size;
        }

    private:
        template<bool>
        friend class Iterator;

        constexpr friend bool operator==(Iterator const& a, Iterator const& b) {
            return a.m_cycle_number == b.m_cycle_number && a.m_base == b.m_base;
        }

        constexpr friend auto operator<=>(Iterator const& a, Iterator const& b)
        requires(concepts::ThreeWayComparable<Iter<is_const>>)
        {
            using Result = meta::CompareThreeWayResult<Iter<is_const>>;
            if (auto result = a.m_cycle_number <=> b.m_cycle_number; result != 0) {
                return static_cast<Result>(result);
            }
            return a.m_base <=> b.m_base;
        }

        constexpr friend SSizeType<is_const> operator-(Iterator const& a, Iterator const& b)
        requires(concepts::RandomAccessIterator<Iter<is_const>>)
        {
            auto start = container::begin(a.m_parent->base_ref());
            auto size = container::distance(start, container::end(a.m_parent->base_ref()));

            return (a.m_cycle_number - b.m_cycle_number) * size + (a.m_base - b.m_base);
        }

        Parent<is_const>* m_parent { nullptr };
        Iter<is_const> m_base {};
        ssize_t m_cycle_number { 0 };
    };

public:
    CycleView()
    requires(concepts::DefaultInitializable<View>)
    = default;

    constexpr CycleView(View view) : m_base(util::move(view)) {}

    constexpr View base() const&
    requires(concepts::CopyConstructible<View>)
    {
        return m_base;
    }

    constexpr View base() &&
        requires(concepts::CopyConstructible<View>)
    {
        return util::move(m_base);
    }

    constexpr View& base_ref() { return m_base; }
    constexpr View const& base_ref() const { return m_base; }

    constexpr auto begin()
    requires(!concepts::SimpleView<View> || !concepts::CommonContainer<View const>)
    {
        return Iterator<false>(this, container::begin(m_base));
    }

    constexpr auto begin() const
    requires(concepts::CommonContainer<View const>)
    {
        return Iterator<true>(this, container::begin(m_base));
    }

    constexpr auto end() const { return unreachable_sentinel; }

private:
    View m_base {};
    [[no_unique_address]] util::StoreIf<util::NonPropagatingCache<ReverseIterator<meta::ContainerIterator<View>>>,
                                        !concepts::CommonContainer<View>>
        m_end_cache;
};

template<typename Con>
CycleView(Con&&) -> CycleView<meta::AsView<Con>>;
}
