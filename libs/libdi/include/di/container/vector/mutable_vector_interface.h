#pragma once

#include <di/concepts/copy_constructible.h>
#include <di/concepts/move_constructible.h>
#include <di/container/vector/constant_vector_interface.h>
#include <di/container/vector/vector_clear.h>
#include <di/container/vector/vector_push_back.h>
#include <di/container/vector/vector_reserve.h>
#include <di/util/move.h>

namespace di::container {
template<typename Self, typename Value>
class MutableVectorInterface : public ConstantVectorInterface<Self> {
private:
    constexpr Self& self() { return static_cast<Self&>(*this); }
    constexpr Self const& self() const { return static_cast<Self const&>(*this); }

public:
    constexpr void clear() { return vector::clear(self()); }

    constexpr Value& push_back(Value const& value)
    requires(concepts::CopyConstructible<Value>)
    {
        return vector::push_back(self(), value);
    }

    constexpr Value& push_back(Value&& value)
    requires(concepts::MoveConstructible<Value>)
    {
        return vector::push_back(self(), util::move(value));
    }

    constexpr auto reserve(size_t n) { return vector::reserve(self(), n); }
};
}
