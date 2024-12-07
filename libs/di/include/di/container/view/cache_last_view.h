#pragma once

#include "di/container/concepts/indirectly_swappable.h"
#include "di/container/concepts/input_container.h"
#include "di/container/interface/begin.h"
#include "di/container/iterator/iterator_base.h"
#include "di/container/iterator/iterator_move.h"
#include "di/container/iterator/iterator_swap.h"
#include "di/container/iterator/sentinel_base.h"
#include "di/container/meta/container_reference.h"
#include "di/container/meta/container_rvalue.h"
#include "di/container/meta/container_ssize_type.h"
#include "di/container/meta/container_value.h"
#include "di/container/types/input_iterator_tag.h"
#include "di/container/view/view_interface.h"
#include "di/function/tag_invoke.h"
#include "di/meta/core.h"
#include "di/meta/language.h"
#include "di/meta/operations.h"
#include "di/util/addressof.h"
#include "di/util/non_propagating_cache.h"

namespace di::container {
namespace detail {
    template<typename T>
    constexpr auto as_lvalue(T&& value) -> T& {
        return value;
    }
}

template<concepts::InputContainer Con>
class CacheLastView : public ViewInterface<CacheLastView<Con>> {
private:
    using Reference = meta::ContainerReference<Con>;
    using It = meta::ContainerIterator<Con>;
    using Sent = meta::ContainerSentinel<Con>;

    using Cache = meta::Conditional<concepts::Reference<Reference>, meta::AddPointer<Reference>, Reference>;

    class Iterator
        : public IteratorBase<Iterator, InputIteratorTag, meta::ContainerValue<Con>, meta::ContainerSSizeType<Con>> {
    private:
        friend class CacheLastView;

        constexpr explicit Iterator(CacheLastView& parent)
            : m_parent(di::addressof(parent)), m_iterator(di::begin(parent.m_container)) {}

    public:
        Iterator(Iterator&&) = default;
        auto operator=(Iterator&&) -> Iterator& = default;

        constexpr auto base() const& -> It const& { return m_iterator; }
        constexpr auto base() && -> It { return di::move(m_iterator); }

        constexpr auto operator*() const -> Reference& {
            if constexpr (concepts::Reference<Reference>) {
                if (!m_parent->m_last_value) {
                    m_parent->m_last_value = di::addressof(detail::as_lvalue(*m_iterator));
                }
                return **m_parent->m_last_value;
            } else {
                if (!m_parent->m_last_value) {
                    m_parent->m_last_value.emplace_deref(m_iterator);
                }
                return *m_parent->m_last_value;
            }
        }

        constexpr void advance_one() {
            ++m_iterator;
            m_parent->m_last_value.reset();
        }

    private:
        constexpr friend auto tag_invoke(di::Tag<iterator_move>, Iterator const& self) -> meta::ContainerRValue<Con> {
            return iterator_move(self.m_iterator);
        }

        constexpr friend void tag_invoke(di::Tag<iterator_swap>, Iterator& a, Iterator& b)
        requires(concepts::IndirectlySwappable<It>)
        {
            iterator_swap(a.m_iterator, b.m_iterator);
        }

        CacheLastView* m_parent;
        It m_iterator;
    };

    class Sentinel : public SentinelBase<Sentinel> {
    private:
        friend class CacheLastView;

        constexpr explicit Sentinel(CacheLastView& parent) : m_sentinel(di::end(parent.m_container)) {}

    public:
        Sentinel() = default;

        constexpr auto base() const -> Sent { return m_sentinel; }

        constexpr auto equals(Iterator const& it) const -> bool { return m_sentinel == it.base(); }

    private:
        Sent m_sentinel {};
    };

public:
    CacheLastView()
    requires(concepts::DefaultInitializable<Con>)
    = default;

    constexpr explicit CacheLastView(Con container) : m_container { di::move(container) } {}

    constexpr auto base() const& -> Con
    requires(concepts::CopyConstructible<Con>)
    {
        return m_container;
    }
    constexpr auto base() && -> Con { return di::move(m_container); }

    constexpr auto begin() { return Iterator(*this); }

    constexpr auto end() { return Sentinel(*this); }

    constexpr auto size()
    requires(concepts::SizedContainer<Con>)
    {
        return m_container.size();
    }
    constexpr auto size() const
    requires(concepts::SizedContainer<Con const>)
    {
        return m_container.size();
    }

private:
    Con m_container {};
    util::NonPropagatingCache<Cache> m_last_value;
};
}
