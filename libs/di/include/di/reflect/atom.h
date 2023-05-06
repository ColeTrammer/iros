#pragma once

#include <di/concepts/constructible_from.h>
#include <di/concepts/copy_constructible.h>
#include <di/concepts/integer.h>
#include <di/concepts/remove_cvref_same_as.h>
#include <di/container/string/constant_string.h>

namespace di::reflection {
template<typename T>
struct Atom {
    using Type = T;

    template<typename U>
    requires(concepts::ConstructibleFrom<T, U> || concepts::RemoveCVRefSameAs<T, U>)
    constexpr static decltype(auto) get(U&& value) {
        if constexpr (concepts::RemoveCVRefSameAs<T, U>) {
            return (util::forward<U>(value));
        } else {
            return T(util::forward<U>(value));
        }
    }

    constexpr static bool is_fields() { return false; }
    constexpr static bool is_field() { return false; }
    constexpr static bool is_atom() { return true; }
    constexpr static bool is_integer() { return concepts::Integer<T>; }
    constexpr static bool is_bool() { return concepts::SameAs<T, bool>; }
    constexpr static bool is_string() { return concepts::detail::ConstantString<T>; }
    constexpr static bool is_list() { return concepts::Container<T> && !is_string() && !is_map(); }
    constexpr static bool is_map() {
        return requires {
            requires concepts::Container<T> && concepts::TupleLike<meta::ContainerValue<T>> &&
                         meta::TupleSize<meta::ContainerValue<T>> == 2;
        };
    }

    bool operator==(Atom const&) const = default;
    auto operator<=>(Atom const&) const = default;
};

template<typename T>
constexpr inline auto atom = Atom<T> {};
}
