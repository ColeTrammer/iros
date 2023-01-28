#pragma once

#include <di/concepts/constructible_from.h>
#include <di/concepts/copy_constructible.h>
#include <di/concepts/default_constructible.h>
#include <di/concepts/move_constructible.h>
#include <di/container/concepts/prelude.h>
#include <di/container/meta/prelude.h>
#include <di/container/vector/constant_vector_interface.h>
#include <di/container/vector/vector_append_container.h>
#include <di/container/vector/vector_clear.h>
#include <di/container/vector/vector_emplace.h>
#include <di/container/vector/vector_emplace_back.h>
#include <di/container/vector/vector_erase.h>
#include <di/container/vector/vector_erase_unstable.h>
#include <di/container/vector/vector_pop_back.h>
#include <di/container/vector/vector_reserve.h>
#include <di/container/vector/vector_resize.h>
#include <di/container/view/clone.h>
#include <di/util/clone.h>
#include <di/util/create.h>
#include <di/util/create_in_place.h>
#include <di/util/forward.h>
#include <di/util/move.h>
#include <di/vocab/expected/prelude.h>

namespace di::container {
template<typename Self, typename Value>
class MutableVectorInterface : public ConstantVectorInterface<Self, Value> {
private:
    constexpr Self& self() { return static_cast<Self&>(*this); }
    constexpr Self const& self() const { return static_cast<Self const&>(*this); }

    using Iterator = Value*;
    using ConstIterator = Value const*;

    template<concepts::InputContainer Con, typename... Args>
    requires(concepts::ContainerCompatible<Con, Value> && concepts::ConstructibleFrom<Self, Args...>)
    constexpr friend auto tag_invoke(types::Tag<util::create_in_place>, InPlaceType<Self>, Con&& container,
                                     Args&&... args) {
        auto result = Self(util::forward<Args>(args)...);
        vector::append_container(result, util::forward<Con>(container));
        return result;
    }

public:
    constexpr auto clone() const
    requires(concepts::Clonable<Value> && concepts::DefaultConstructible<Self>)
    {
        return util::create<Self>(*this | view::clone);
    }

    constexpr void clear() { return vector::clear(self()); }

    constexpr decltype(auto) push_back(Value const& value)
    requires(concepts::CopyConstructible<Value>)
    {
        return vector::emplace_back(self(), value);
    }

    constexpr decltype(auto) push_back(Value&& value)
    requires(concepts::MoveConstructible<Value>)
    {
        return vector::emplace_back(self(), util::move(value));
    }

    template<typename... Args>
    requires(concepts::ConstructibleFrom<Value, Args...>)
    constexpr decltype(auto) emplace_back(Args&&... args) {
        return vector::emplace_back(self(), util::forward<Args>(args)...);
    }

    template<concepts::InputContainer Con>
    requires(concepts::ContainerCompatible<Con, Value>)
    constexpr auto append_container(Con&& container) {
        return vector::append_container(self(), util::forward<Con>(container));
    }

    constexpr auto insert(ConstIterator position, Value const& value)
    requires(concepts::CopyConstructible<Value>)
    {
        return vector::emplace(self(), position, value);
    }

    constexpr auto insert(ConstIterator position, Value&& value)
    requires(concepts::MoveConstructible<Value>)
    {
        return vector::emplace(self(), position, util::move(value));
    }

    template<typename... Args>
    requires(concepts::ConstructibleFrom<Value, Args...>)
    constexpr auto emplace(ConstIterator position, Args&&... args) {
        return vector::emplace(self(), position, util::forward<Args>(args)...);
    }

    constexpr auto pop_back() { return vector::pop_back(self()); }

    constexpr auto erase(ConstIterator position) { return vector::erase(self(), position); }
    constexpr auto erase(ConstIterator start, ConstIterator end) { return vector::erase(self(), start, end); }

    constexpr auto erase_unstable(ConstIterator iter) { return vector::erase_unstable(self(), iter); }

    constexpr auto resize(size_t count)
    requires(concepts::DefaultConstructible<Value>)
    {
        return vector::resize(self(), count);
    }

    constexpr auto resize(size_t count, Value const& value)
    requires(concepts::CopyConstructible<Value>)
    {
        return vector::resize(self(), count, value);
    }

    using ConstantVectorInterface<Self, Value>::iterator;
    constexpr auto iterator(ConstIterator iter) { return vector::iterator(self(), iter); }

    constexpr auto reserve(size_t n) { return vector::reserve(self(), n); }
};
}
