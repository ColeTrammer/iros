#pragma once

#include <di/container/action/to.h>
#include <di/container/concepts/prelude.h>
#include <di/container/iterator/iterator_base.h>
#include <di/container/meta/prelude.h>
#include <di/container/vector/vector.h>
#include <di/util/deduce_create.h>
#include <di/vocab/optional/prelude.h>

namespace di::container {
namespace detail {
    template<typename Con, typename Value>
    concept StackCompatible =
        concepts::Container<Con> && concepts::SameAs<Value, meta::ContainerValue<Con>> && requires(Con& container, Value&& value) {
                                                                                              container.back();
                                                                                              util::as_const(container).back();
                                                                                              container.emplace_back(util::move(value));
                                                                                              container.pop_back();
                                                                                              {
                                                                                                  container.size()
                                                                                                  } -> concepts::UnsignedInteger;
                                                                                          };
}

template<typename Value, detail::StackCompatible<Value> Con = container::Vector<Value>>
class Stack {
private:
    template<concepts::InputContainer Other>
    requires(concepts::ContainerCompatible<Other, Value>)
    constexpr friend auto tag_invoke(types::Tag<util::create_in_place>, InPlaceType<Stack>, Other&& other) {
        return as_fallible(util::forward<Other>(other) | container::to<Con>()) % [&](Con&& container) {
            return Stack(util::move(container));
        } | try_infallible;
    }

    struct Iterator : public IteratorBase<Iterator, Value, meta::ContainerSSizeType<Con>> {
    private:
        friend class Stack;

        constexpr explicit Iterator(Stack& base) : m_base(util::address_of(base)) {}

    public:
        Iterator() = default;
        Iterator(Iterator const&) = delete;
        Iterator(Iterator&&) = default;

        Iterator& operator=(Iterator const&) = delete;
        Iterator& operator=(Iterator&&) = default;

        constexpr Value& operator*() const { return *m_base->top(); }

        constexpr void advance_one() { m_base->pop(); }

    private:
        constexpr friend bool operator==(Iterator const& a, DefaultSentinel const&) { return a.m_base->empty(); }

        constexpr friend auto tag_invoke(types::Tag<iterator_category>, InPlaceType<Iterator>) { return types::InputIteratorTag {}; }

        Stack* m_base { nullptr };
    };

public:
    Stack() = default;

    constexpr explicit Stack(Con&& container) : m_container(util::move(container)) {}

    constexpr Optional<Value&> top() { return m_container.back(); }
    constexpr Optional<Value const&> top() const { return m_container.back(); }

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

    constexpr auto pop() { return m_container.pop_back(); }

    constexpr auto begin() { return Iterator(*this); }
    constexpr auto end() { return default_sentinel; }

    constexpr Con const& base() const { return m_container; }

private:
    constexpr friend auto tag_invoke(types::Tag<util::clone>, Stack const& self) { return self | container::to<Stack>(); }

    Con m_container {};
};

template<concepts::InputContainer Con, typename T = meta::ContainerValue<Con>>
Stack<T> tag_invoke(types::Tag<util::deduce_create>, InPlaceTemplate<Stack>, Con&&);
}