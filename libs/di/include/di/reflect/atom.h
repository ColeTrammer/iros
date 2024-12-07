#pragma once

#include "di/container/string/constant_string.h"
#include "di/meta/language.h"
#include "di/meta/operations.h"
#include "di/meta/util.h"

namespace di::reflection {
template<typename T>
struct Atom {
    using Type = T;

    template<typename U>
    requires(concepts::ConstructibleFrom<T, U> || concepts::RemoveCVRefSameAs<T, U>)
    constexpr static auto get(U&& value) -> decltype(auto) {
        if constexpr (concepts::RemoveCVRefSameAs<T, U>) {
            return (util::forward<U>(value));
        } else {
            return T(util::forward<U>(value));
        }
    }

    constexpr static auto is_fields() -> bool { return false; }
    constexpr static auto is_field() -> bool { return false; }
    constexpr static auto is_enumerator() -> bool { return false; }
    constexpr static auto is_enumerators() -> bool { return false; }
    constexpr static auto is_atom() -> bool { return true; }
    constexpr static auto is_integer() -> bool { return concepts::Integer<T>; }
    constexpr static auto is_bool() -> bool { return concepts::SameAs<T, bool>; }
    constexpr static auto is_string() -> bool { return concepts::detail::ConstantString<T>; }
    constexpr static auto is_list() -> bool { return concepts::Container<T> && !is_string() && !is_map(); }
    constexpr static auto is_map() -> bool {
        return requires {
            requires concepts::Container<T> && concepts::TupleLike<meta::ContainerValue<T>> &&
                         meta::TupleSize<meta::ContainerValue<T>> == 2;
        };
    }

    auto operator==(Atom const&) const -> bool = default;
    auto operator<=>(Atom const&) const = default;
};

template<typename T>
constexpr inline auto atom = Atom<T> {};
}

namespace di {
using reflection::Atom;
using reflection::atom;
}
