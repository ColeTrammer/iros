#pragma once

#include <di/meta/language.h>
#include <di/util/addressof.h>
#include <di/vocab/optional/prelude.h>

namespace di::util {
template<concepts::Object T>
class NonPropagatingCache : public Optional<T> {
public:
    using Parent = Optional<T>;
    using Parent::Parent;

    constexpr NonPropagatingCache(NonPropagatingCache const&) {}
    constexpr NonPropagatingCache(NonPropagatingCache&& other) { other.reset(); }

    template<concepts::ConvertibleTo<T> U>
    constexpr NonPropagatingCache& operator=(U&& value) {
        this->emplace(util::forward<U>(value));
        return *this;
    }

    constexpr NonPropagatingCache& operator=(NonPropagatingCache const& other) {
        if (util::addressof(other) != this) {
            this->reset();
        }
        return *this;
    }
    constexpr NonPropagatingCache& operator=(NonPropagatingCache&& other) {
        this->reset();
        other.reset();
        return *this;
    }

    template<typename I>
    constexpr T& emplace_deref(I const& it)
    requires(requires { T(*it); })
    {
        this->reset();
        return this->emplace(*it);
    }
};
}
