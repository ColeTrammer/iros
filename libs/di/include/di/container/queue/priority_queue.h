#pragma once

#include "di/container/action/sequence.h"
#include "di/container/action/to.h"
#include "di/container/algorithm/make_heap.h"
#include "di/container/algorithm/pop_heap.h"
#include "di/container/algorithm/push_heap.h"
#include "di/container/concepts/prelude.h"
#include "di/container/iterator/iterator_base.h"
#include "di/container/meta/prelude.h"
#include "di/container/vector/vector.h"
#include "di/util/deduce_create.h"
#include "di/vocab/optional/prelude.h"

namespace di::container {
namespace detail {
    template<typename Con, typename Value>
    concept PriorityQueueCompatible =
        concepts::RandomAccessContainer<Con> && concepts::Permutable<meta::ContainerIterator<Con>> &&
        concepts::SameAs<Value, meta::ContainerValue<Con>> && requires(Con& container, Value&& value) {
            { container.front() } -> concepts::SameAs<Optional<Value&>>;
            { util::as_const(container).front() } -> concepts::SameAs<Optional<Value const&>>;
            container.emplace_back(util::move(value));
            { container.pop_back() } -> concepts::SameAs<Optional<Value>>;
            { container.size() } -> concepts::UnsignedInteger;
        };
}

template<typename Value, detail::PriorityQueueCompatible<Value> Con = container::Vector<Value>,
         concepts::StrictWeakOrder<Value> Comp = function::Compare>
class PriorityQueue {
private:
    template<concepts::InputContainer Other>
    requires(concepts::ContainerCompatible<Other, Value>)
    constexpr friend auto tag_invoke(types::Tag<util::create_in_place>, InPlaceType<PriorityQueue>, Other&& other,
                                     Comp const& comp = {}) {
        return as_fallible(util::forward<Other>(other) | container::to<Con>()) % [&](Con&& container) {
            return PriorityQueue(comp, util::move(container));
        } | try_infallible;
    }

    struct Iterator : public IteratorBase<Iterator, InputIteratorTag, Value, meta::ContainerSSizeType<Con>> {
    private:
        friend class PriorityQueue;

        constexpr explicit Iterator(PriorityQueue& base) : m_base(util::addressof(base)) {}

    public:
        Iterator() = default;

        constexpr auto operator*() const -> Value& { return *m_base->top(); }

        constexpr void advance_one() { m_base->pop(); }

    private:
        constexpr friend auto operator==(Iterator const& a, DefaultSentinel const&) -> bool {
            return a.m_base->empty();
        }

        PriorityQueue* m_base { nullptr };
    };

public:
    PriorityQueue() = default;

    constexpr explicit PriorityQueue(Comp const& compare) : m_comp(compare) {}

    constexpr explicit PriorityQueue(Comp const& compare, Con&& container)
        : m_container(util::move(container)), m_comp(compare) {
        container::make_heap(m_container, util::ref(m_comp));
    }

    constexpr auto top() -> Optional<Value&> { return m_container.front(); }
    constexpr auto top() const -> Optional<Value const&> { return m_container.front(); }

    constexpr auto empty() const -> bool { return size() == 0U; }
    constexpr auto size() const { return m_container.size(); }

    constexpr auto push(Value const& value) -> decltype(auto)
    requires(concepts::CopyConstructible<Value>)
    {
        return emplace(value);
    }
    constexpr auto push(Value&& value) -> decltype(auto) { return emplace(util::move(value)); }

    template<typename... Args>
    requires(concepts::ConstructibleFrom<Value, Args...>)
    constexpr auto emplace(Args&&... args) -> decltype(auto) {
        return as_fallible(m_container.emplace_back(util::forward<Args>(args)...)) | if_success([&](auto&&...) {
                   container::push_heap(m_container, util::ref(m_comp));
               }) |
               try_infallible;
    }

    template<concepts::ContainerCompatible<Value> Other>
    constexpr auto push_container(Other&& container) {
        auto old_size = size();
        return invoke_as_fallible([&] {
                   return m_container.append_container(util::forward<Other>(container));
               }) % [&] {
            for (auto i = old_size + 1U; i <= size(); i++) {
                container::push_heap(m_container.begin(), m_container.begin() + i, util::ref(m_comp));
            }
        } | try_infallible;
    }

    constexpr auto pop() -> Optional<Value> {
        if (empty()) {
            return nullopt;
        }
        auto value = util::move(m_container[0]);
        container::pop_heap(m_container, util::ref(m_comp));
        m_container.pop_back();
        return value;
    }

    constexpr auto begin() { return Iterator(*this); }
    constexpr auto end() { return default_sentinel; }

    constexpr auto base() const -> Con const& { return m_container; }
    constexpr auto comparator() const -> Comp const& { return m_comp; }

    constexpr void clear() { m_container.clear(); }

private:
    constexpr explicit PriorityQueue(InPlace, Con&& container, Comp const& comp)
        : m_container(util::move(container)), m_comp(comp) {}

    constexpr friend auto tag_invoke(types::Tag<util::clone>, PriorityQueue const& self) {
        return as_fallible(util::clone(self.m_container)) % [&](Con&& container) {
            return PriorityQueue(in_place, util::move(container), self.m_comp);
        } | try_infallible;
    }

    Con m_container {};
    [[no_unique_address]] Comp m_comp {};
};

template<concepts::Container Con, typename T = meta::ContainerValue<Con>, concepts::StrictWeakOrder<T> Comp>
requires(detail::PriorityQueueCompatible<Con, T>)
PriorityQueue(Comp, Con) -> PriorityQueue<T, Con, Comp>;

template<concepts::InputContainer Con, typename T = meta::ContainerValue<Con>>
auto tag_invoke(types::Tag<util::deduce_create>, InPlaceTemplate<PriorityQueue>, Con&&) -> PriorityQueue<T>;

template<concepts::InputContainer Con, typename T = meta::ContainerValue<Con>, concepts::StrictWeakOrder<T> Comp>
auto tag_invoke(types::Tag<util::deduce_create>, InPlaceTemplate<PriorityQueue>, Con&&, Comp)
    -> PriorityQueue<T, container::Vector<T>, Comp>;
}

namespace di {
using container::PriorityQueue;
}
