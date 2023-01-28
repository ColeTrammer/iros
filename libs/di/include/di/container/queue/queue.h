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
                                  container.pop_front();
                              };
}

template<typename Value, detail::QueueCompatible<Value> Con = container::LinkedList<Value>>
class Queue {
private:
    Con m_container {};

    template<concepts::InputContainer Other>
    requires(concepts::ContainerCompatible<Other, Value>)
    constexpr friend auto tag_invoke(types::Tag<util::create_in_place>, InPlaceType<Queue>, Other&& other) {
        return as_fallible(util::forward<Other>(other) | container::to<Con>()) % [&](Con&& container) {
            return Queue(util::move(container));
        } | try_infallible;
    }

    struct Iterator : public IteratorBase<Iterator, InputIteratorTag, Value, meta::ContainerSSizeType<Con>> {
    private:
        friend class Queue;

        constexpr explicit Iterator(Queue& base) : m_base(util::address_of(base)) {}

    public:
        Iterator() = default;

        constexpr Value& operator*() const { return *m_base->front(); }

        constexpr void advance_one() { m_base->pop(); }

    private:
        constexpr friend bool operator==(Iterator const& a, DefaultSentinel const&) { return a.m_base->empty(); }

        Queue* m_base { nullptr };
    };

public:
    Queue() = default;

    Queue(Queue&&) = default;

    constexpr explicit Queue(Con&& container) : m_container(util::move(container)) {}

    ~Queue() = default;

    Queue& operator=(Queue&&) = default;

    constexpr auto front() { return m_container.front(); }
    constexpr auto front() const { return m_container.front(); }

    constexpr auto back()
    requires(requires { m_container.back(); })
    {
        return m_container.back();
    }
    constexpr auto back() const
    requires(requires { m_container.back(); })
    {
        return m_container.back();
    }

    constexpr bool empty() const { return m_container.empty(); }
    constexpr auto size() const
    requires(concepts::SizedContainer<Con>)
    {
        return m_container.size();
    }

    constexpr decltype(auto) push(Value& value)
    requires(!concepts::CopyConstructible<Value> && !concepts::MoveConstructible<Value>)
    {
        return m_container.push_back(value);
    }

    constexpr decltype(auto) push(Value const& value)
    requires(concepts::CopyConstructible<Value>)
    {
        return m_container.push_back(value);
    }

    constexpr decltype(auto) push(Value&& value)
    requires(concepts::MoveConstructible<Value>)
    {
        return m_container.push_back(util::move(value));
    }

    template<typename... Args>
    constexpr decltype(auto) emplace(Args&&... args)
    requires(requires { m_container.emplace_back(util::forward<Args>(args)...); })
    {
        return m_container.emplace_back(util::forward<Args>(args)...);
    }

    template<concepts::ContainerCompatible<Value> Other>
    requires(requires { m_container.append_container(util::forward<Other>(m_container)); })
    constexpr auto push_container(Other&& container) {
        return m_container.append_container(util::forward<Other>(container));
    }

    constexpr auto pop() { return m_container.pop_front(); }

    constexpr auto begin() { return Iterator(*this); }
    constexpr auto end() { return default_sentinel; }

    constexpr Con const& base() const { return m_container; }

private:
    constexpr friend auto tag_invoke(types::Tag<util::clone>, Queue const& self)
    requires(concepts::Clonable<Value>)
    {
        return self | container::to<Queue>();
    }
};

template<concepts::Container Con, typename T = meta::ContainerValue<Con>>
requires(detail::QueueCompatible<Con, T>)
Queue(Con) -> Queue<T, Con>;

template<concepts::InputContainer Con, typename T = meta::ContainerValue<Con>>
Queue<T> tag_invoke(types::Tag<util::deduce_create>, InPlaceTemplate<Queue>, Con&&);
}