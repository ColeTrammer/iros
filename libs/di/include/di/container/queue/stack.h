#pragma once

#include <di/container/action/to.h>
#include <di/container/concepts/prelude.h>
#include <di/container/iterator/iterator_base.h>
#include <di/container/meta/prelude.h>
#include <di/container/vector/vector.h>
#include <di/container/view/as_rvalue.h>
#include <di/util/deduce_create.h>
#include <di/vocab/optional/prelude.h>

namespace di::container {
namespace detail {
    template<typename Con, typename Value>
    concept StackCompatible = concepts::Container<Con> && concepts::SameAs<Value, meta::ContainerValue<Con>> &&
                              requires(Con& container, Value&& value) {
                                  { container.back() } -> concepts::SameAs<Optional<Value&>>;
                                  { util::as_const(container).back() } -> concepts::SameAs<Optional<Value const&>>;
                                  container.emplace_back(util::move(value));
                                  { container.pop_back() } -> concepts::SameAs<Optional<Value>>;
                                  { container.size() } -> concepts::UnsignedInteger;
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

    struct Iterator : public IteratorBase<Iterator, InputIteratorTag, Value, meta::ContainerSSizeType<Con>> {
    private:
        friend class Stack;

        constexpr explicit Iterator(Stack& base) : m_base(util::addressof(base)) {}

    public:
        Iterator() = default;

        constexpr auto operator*() const -> Value& { return *m_base->top(); }

        constexpr void advance_one() { m_base->pop(); }

    private:
        constexpr friend auto operator==(Iterator const& a, DefaultSentinel const&) -> bool {
            return a.m_base->empty();
        }

        Stack* m_base { nullptr };
    };

public:
    Stack() = default;

    constexpr explicit Stack(Con&& container) : m_container(util::move(container)) {}

    constexpr auto top() -> Optional<Value&> { return m_container.back(); }
    constexpr auto top() const -> Optional<Value const&> { return m_container.back(); }

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
        return m_container.emplace_back(util::forward<Args>(args)...);
    }

    template<concepts::ContainerCompatible<Value> Other>
    constexpr auto push_container(Other&& container) {
        return m_container.append_container(util::forward<Other>(container));
    }

    constexpr auto pop() { return m_container.pop_back(); }

    constexpr auto begin() { return Iterator(*this); }
    constexpr auto end() { return default_sentinel; }

    constexpr auto base() const -> Con const& { return m_container; }

    constexpr void clear() { m_container.clear(); }

private:
    constexpr friend auto tag_invoke(types::Tag<util::clone>, Stack const& self) {
        return self | container::to<Stack>();
    }

    Con m_container {};
};

template<concepts::Container Con, typename T = meta::ContainerValue<Con>>
requires(detail::StackCompatible<Con, T>)
Stack(Con) -> Stack<T, Con>;

template<concepts::InputContainer Con, typename T = meta::ContainerValue<Con>>
auto tag_invoke(types::Tag<util::deduce_create>, InPlaceTemplate<Stack>, Con&&) -> Stack<T>;
}

namespace di {
using container::Stack;
}
