#pragma once

#include <di/container/algorithm/search.h>
#include <di/container/concepts/prelude.h>
#include <di/container/interface/prelude.h>
#include <di/container/meta/prelude.h>
#include <di/container/view/single.h>
#include <di/container/view/view_interface.h>
#include <di/function/equal.h>
#include <di/util/move.h>
#include <di/vocab/optional/prelude.h>

namespace di::container {
template<concepts::ForwardContainer View, concepts::ForwardContainer Pattern>
requires(
    concepts::View<View> && concepts::View<Pattern> &&
    concepts::IndirectlyComparable<meta::ContainerIterator<View>, meta::ContainerIterator<Pattern>, function::Equal>)
class SplitView : public ViewInterface<SplitView<View, Pattern>> {
private:
    friend struct Sentinel;
    friend struct Iterator;

    using Value = meta::Reconstructed<View, meta::ContainerIterator<View>, meta::ContainerIterator<View>>;

    struct Iterator : public IteratorBase<Iterator, ForwardIteratorTag, Value, meta::ContainerSSizeType<View>> {
    public:
        Iterator() = default;

        constexpr Iterator(SplitView& parent, meta::ContainerIterator<View> base, Value next)
            : m_parent(util::addressof(parent)), m_base(util::move(base)), m_next(util::move(next)) {}

        constexpr auto base() const { return m_base; }

        constexpr auto operator*() const -> Value {
            return container::reconstruct(in_place_type<View>, m_base, container::begin(m_next));
        }

        constexpr void advance_one() {
            m_base = container::begin(m_next);
            if (m_base != container::end(m_parent->m_base)) {
                m_base = container::end(m_next);
                if (m_base == container::end(m_parent->m_base)) {
                    m_trailing_empty = true;
                    m_next = container::reconstruct(in_place_type<View>, m_base, m_base);
                } else {
                    m_next = m_parent->find_next(m_base);
                }
            } else {
                m_trailing_empty = false;
            }
        }

    private:
        friend struct Sentinel;

        constexpr friend auto operator==(Iterator const& a, Iterator const& b) -> bool {
            return a.m_base == b.m_base && a.m_trailing_empty == b.m_trailing_empty;
        }

        SplitView* m_parent { nullptr };
        meta::ContainerIterator<View> m_base;
        Value m_next;
        bool m_trailing_empty { false };
    };

    struct Sentinel {
    public:
        Sentinel() = default;

        constexpr explicit Sentinel(SplitView& parent) : m_base(container::end(parent.m_base)) {}

    private:
        constexpr friend auto operator==(Iterator const& a, Sentinel const& b) -> bool {
            return a.m_base == b.m_base && !a.m_trailing_empty;
        }

        meta::ContainerSentinel<View> m_base;
    };

public:
    SplitView()
    requires(concepts::DefaultInitializable<View> && concepts::DefaultInitializable<Pattern>)
    = default;

    constexpr SplitView(View base, Pattern pattern) : m_base(util::move(base)), m_pattern(util::move(pattern)) {}

    template<concepts::InputContainer Con>
    requires(concepts::ConstructibleFrom<View, meta::AsView<Con>> &&
             concepts::ConstructibleFrom<Pattern, SingleView<meta::ContainerValue<Con>>>)
    constexpr SplitView(Con&& container, meta::ContainerValue<Con> value)
        : m_base(view::all(util::forward<Con>(container))), m_pattern(SingleView { util::move(value) }) {}

    constexpr auto base() const& -> View
    requires(concepts::CopyConstructible<View>)
    {
        return m_base;
    }
    constexpr auto base() && -> View { return util::move(m_base); }

    constexpr auto begin() -> Iterator {
        if (!m_cached_begin) {
            m_cached_begin = this->find_next(container::begin(m_base));
        }
        return { *this, container::begin(m_base), *m_cached_begin };
    }

    constexpr auto end() {
        if constexpr (concepts::CommonContainer<View>) {
            return Iterator { *this, container::end(m_base),
                              container::reconstruct(in_place_type<View>, container::end(m_base),
                                                     container::end(m_base)) };
        } else {
            return Sentinel { *this };
        }
    }

private:
    constexpr auto find_next(meta::ContainerIterator<View> it) -> Value {
        auto [start, end] = container::search(container::View(it, container::end(m_base)), m_pattern);
        if (start != container::end(m_base) && container::empty(m_pattern)) {
            ++start;
            ++end;
        }
        return container::reconstruct(in_place_type<View>, util::move(start), util::move(end));
    }

    View m_base;
    Pattern m_pattern;
    Optional<Value> m_cached_begin;
};

template<typename Con, typename Pattern>
SplitView(Con&&, Pattern&&) -> SplitView<meta::AsView<Con>, meta::AsView<Pattern>>;

template<concepts::InputContainer Con>
SplitView(Con&&, meta::ContainerValue<Con>) -> SplitView<meta::AsView<Con>, SingleView<meta::ContainerValue<Con>>>;
}
