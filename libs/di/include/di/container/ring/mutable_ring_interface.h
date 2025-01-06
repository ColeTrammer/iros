#pragma once

#include "di/container/ring/constant_ring_interface.h"
#include "di/container/vector/mutable_vector_interface.h"

namespace di::container {
template<typename Self, typename Value>
class MutableRingInterface : public ConstantRingInterface<Self, Value> {
private:
    constexpr auto self() -> Self& { return static_cast<Self&>(*this); }
    constexpr auto self() const -> Self const& { return static_cast<Self const&>(*this); }

    using Iterator = RingIterator<Value>;
    using ConstIterator = RingIterator<Value const>;

    template<concepts::InputContainer Con, typename... Args>
    requires(concepts::ContainerCompatible<Con, Value> && concepts::ConstructibleFrom<Self, Args...>)
    constexpr friend auto tag_invoke(types::Tag<util::create_in_place>, InPlaceType<Self>, Con&& container,
                                     Args&&... args) {
        auto result = Self(util::forward<Args>(args)...);
        return invoke_as_fallible([&] {
                   return vector::append_container(result, util::forward<Con>(container));
               }) % [&] {
            result.assume_tail(result.size());
            return util::move(result);
        } | try_infallible;
    }

public:
    constexpr auto clone() const
    requires(concepts::Clonable<Value> && concepts::DefaultConstructible<Self>)
    {
        return util::create<Self>(*this | view::clone);
    }

    constexpr void clear() { ring::clear(self()); }

    constexpr auto push_back(Value const& value) -> decltype(auto)
    requires(concepts::CopyConstructible<Value>)
    {
        return ring::emplace_back(self(), value);
    }

    constexpr auto push_back(Value&& value) -> decltype(auto)
    requires(concepts::MoveConstructible<Value>)
    {
        return ring::emplace_back(self(), util::move(value));
    }

    template<typename... Args>
    requires(concepts::ConstructibleFrom<Value, Args...>)
    constexpr auto emplace_back(Args&&... args) -> decltype(auto) {
        return ring::emplace_back(self(), util::forward<Args>(args)...);
    }

    template<concepts::InputContainer Con>
    requires(concepts::ContainerCompatible<Con, Value>)
    constexpr auto append_container(Con&& container) {
        return ring::append_container(self(), util::forward<Con>(container));
    }

    constexpr auto pop_back() -> decltype(auto) { return ring::pop_back(self()); }

    constexpr auto push_front(Value const& value) -> decltype(auto)
    requires(concepts::CopyConstructible<Value>)
    {
        return ring::emplace_front(self(), value);
    }

    constexpr auto push_front(Value&& value) -> decltype(auto)
    requires(concepts::MoveConstructible<Value>)
    {
        return ring::emplace_front(self(), util::move(value));
    }

    template<typename... Args>
    requires(concepts::ConstructibleFrom<Value, Args...>)
    constexpr auto emplace_front(Args&&... args) -> decltype(auto) {
        return ring::emplace_front(self(), util::forward<Args>(args)...);
    }

    template<concepts::InputContainer Con>
    requires(concepts::ContainerCompatible<Con, Value>)
    constexpr auto prepend_container(Con&& container) {
        return ring::prepend_container(self(), util::forward<Con>(container));
    }

    constexpr auto pop_front() -> decltype(auto) { return ring::pop_front(self()); }

    constexpr auto insert(ConstIterator position, Value const& value)
    requires(concepts::CopyConstructible<Value>)
    {
        return ring::emplace(self(), position, value);
    }

    constexpr auto insert(ConstIterator position, Value&& value)
    requires(concepts::MoveConstructible<Value>)
    {
        return ring::emplace(self(), position, util::move(value));
    }

    template<typename... Args>
    requires(concepts::ConstructibleFrom<Value, Args...>)
    constexpr auto emplace(ConstIterator position, Args&&... args) {
        return ring::emplace(self(), position, util::forward<Args>(args)...);
    }

    constexpr auto erase(ConstIterator position) { return ring::erase(self(), position); }
    constexpr auto erase(ConstIterator start, ConstIterator end) { return ring::erase(self(), start, end); }

    using ConstantRingInterface<Self, Value>::iterator;
    constexpr auto iterator(ConstIterator iter) { return ring::iterator(self(), iter); }

    constexpr auto reserve(usize n) { return ring::reserve(self(), n); }

    constexpr auto make_contigous() { return ring::make_contigous(self()); }

    constexpr auto resize(size_t count)
    requires(concepts::DefaultConstructible<Value>)
    {
        return ring::resize(self(), count);
    }

    constexpr auto resize(size_t count, Value const& value)
    requires(concepts::CopyConstructible<Value>)
    {
        return ring::resize(self(), count, value);
    }

private:
    template<typename F, SameAs<Tag<erase_if>> T = Tag<erase_if>>
    requires(concepts::Predicate<F, Value const&>)
    constexpr friend auto tag_invoke(T, Self& self, F&& function) {
        auto [first, last] = remove_if(self, di::forward<F>(function));
        auto const count = usize(last - first);

        ring::erase(self, first, last);
        return count;
    }
};
}
