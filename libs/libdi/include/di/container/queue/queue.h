#pragma once

#include <di/container/action/to.h>
#include <di/container/concepts/prelude.h>
#include <di/container/iterator/iterator_base.h>
#include <di/container/linked/linked_list.h>
#include <di/container/meta/prelude.h>
#include <di/util/deduce_create.h>
#include <di/vocab/optional/prelude.h>

namespace di::container {
namespace detail {
    template<typename Con, typename Value>
    concept QueueCompatible = concepts::Container<Con> && concepts::SameAs<Value, meta::ContainerValue<Con>> &&
                              requires(Con& container, Value&& value) {
                                  { container.front() } -> concepts::SameAs<Optional<Value&>>;
                                  { util::as_const(container).front() } -> concepts::SameAs<Optional<Value const&>>;
                                  { container.back() } -> concepts::SameAs<Optional<Value&>>;
                                  { util::as_const(container).back() } -> concepts::SameAs<Optional<Value const&>>;
                                  container.emplace_back(util::move(value));
                                  { container.append_container(util::move(container)) } -> concepts::MaybeFallible<void>;
                                  { container.pop_front() } -> concepts::SameAs<Optional<Value>>;
                                  { container.size() } -> concepts::UnsignedInteger;
                              };
}

template<typename Value, detail::QueueCompatible<Value> Con = container::LinkedList<Value>>
class Queue {
private:
    template<concepts::InputContainer Other>
    requires(concepts::ContainerCompatible<Other, Value>)
    constexpr friend auto tag_invoke(types::Tag<util::create_in_place>, InPlaceType<Queue>, Other&& other) {
        return as_fallible(util::forward<Other>(other) | container::to<Con>()) % [&](Con&& container) {
            return Queue(util::move(container));
        } | try_infallible;
    }

    struct Iterator : public IteratorBase<Iterator, Value, meta::ContainerSSizeType<Con>> {
    private:
        friend class Queue;

        constexpr explicit Iterator(Queue& base) : m_base(util::address_of(base)) {}

    public:
        Iterator() = default;
        Iterator(Iterator const&) = delete;
        Iterator(Iterator&&) = default;

        Iterator& operator=(Iterator const&) = delete;
        Iterator& operator=(Iterator&&) = default;

        constexpr Value& operator*() const { return *m_base->front(); }

        constexpr void advance_one() { m_base->pop(); }

    private:
        constexpr friend bool operator==(Iterator const& a, DefaultSentinel const&) { return a.m_base->empty(); }

        constexpr friend auto tag_invoke(types::Tag<iterator_category>, InPlaceType<Iterator>) { return types::InputIteratorTag {}; }

        Queue* m_base { nullptr };
    };

public:
    Queue() = default;

    Queue(Queue&&) = default;

    constexpr explicit Queue(Con&& container) : m_container(util::move(container)) {}

    ~Queue() = default;

    Queue& operator=(Queue&&) = default;

    constexpr Optional<Value&> front() { return m_container.front(); }
    constexpr Optional<Value const&> front() const { return m_container.front(); }

    constexpr Optional<Value&> back() { return m_container.back(); }
    constexpr Optional<Value const&> back() const { return m_container.back(); }

    constexpr bool empty() const { return size() == 0u; }
    constexpr auto size() const { return m_container.size(); }

    constexpr decltype(auto) push(Value const& value)
    requires(concepts::CopyConstructible<Value>)
    {
        return emplace(value);
    }
    constexpr decltype(auto) push(Value&& value) { return emplace(util::move(value)); }

    template<typename... Args>
    requires(concepts::ConstructibleFrom<Value, Args...>)
    constexpr decltype(auto) emplace(Args&&... args) {
        return m_container.emplace_back(util::forward<Args>(args)...);
    }

    template<concepts::ContainerCompatible<Value> Other>
    constexpr auto push_container(Other&& container) {
        return m_container.append_container(util::forward<Other>(container));
    }

    constexpr auto pop() { return m_container.pop_front(); }

    constexpr auto begin() { return Iterator(*this); }
    constexpr auto end() { return default_sentinel; }

    constexpr Con const& base() const { return m_container; }

private:
    constexpr friend auto tag_invoke(types::Tag<util::clone>, Queue const& self) { return self | container::to<Queue>(); }

    Con m_container {};
};

template<concepts::Container Con, typename T = meta::ContainerValue<Con>>
requires(detail::QueueCompatible<Con, T>)
Queue(Con) -> Queue<T, Con>;

template<concepts::InputContainer Con, typename T = meta::ContainerValue<Con>>
Queue<T> tag_invoke(types::Tag<util::deduce_create>, InPlaceTemplate<Queue>, Con&&);
}