#pragma once

#include "di/meta/language.h"
#include "di/util/addressof.h"
#include "di/vocab/optional/prelude.h"

namespace di::util {
template<concepts::Object T>
class NonPropagatingCache : public Optional<T> {
public:
    using Parent = Optional<T>;
    using Parent::Parent;

    constexpr NonPropagatingCache(NonPropagatingCache const&) : Optional<T>() {}
    constexpr NonPropagatingCache(NonPropagatingCache&& other) : Optional<T>() { other.reset(); }

    template<concepts::ConvertibleTo<T> U>
    constexpr auto operator=(U&& value) -> NonPropagatingCache& {
        this->emplace(util::forward<U>(value));
        return *this;
    }

    constexpr auto operator=(NonPropagatingCache const& other) -> NonPropagatingCache& {
        if (util::addressof(other) != this) {
            this->reset();
        }
        return *this;
    }
    constexpr auto operator=(NonPropagatingCache&& other) -> NonPropagatingCache& {
        this->reset();
        other.reset();
        return *this;
    }

    template<typename I>
    constexpr auto emplace_deref(I const& it) -> T& requires(requires { T(*it); }) {
        this->reset();
        return this->emplace(*it);
    }
};
}

namespace di {
using util::NonPropagatingCache;
}
