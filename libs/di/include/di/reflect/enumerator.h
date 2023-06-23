#pragma once

#include <di/concepts/enum.h>
#include <di/container/string/fixed_string.h>
#include <di/meta/core.h>
#include <di/vocab/tuple/tuple.h>

namespace di::reflection {
template<container::FixedString enumerator_name, auto enumerator_value>
requires(concepts::Enum<decltype(enumerator_value)>)
struct Enumerator {
    constexpr static auto name = enumerator_name;
    constexpr static auto value = enumerator_value;

    using Type = decltype(enumerator_value);

    constexpr static Type get() { return value; }

    constexpr static bool is_fields() { return false; }
    constexpr static bool is_field() { return false; }
    constexpr static bool is_enumerator() { return true; }
    constexpr static bool is_enumerators() { return false; }
    constexpr static bool is_atom() { return false; }
    constexpr static bool is_integer() { return false; }
    constexpr static bool is_bool() { return false; }
    constexpr static bool is_string() { return false; }
    constexpr static bool is_list() { return false; }
    constexpr static bool is_map() { return false; }

    bool operator==(Enumerator const&) const = default;
    auto operator<=>(Enumerator const&) const = default;
};

template<container::FixedString enumerator_name, auto enumerator_value>
requires(concepts::Enum<decltype(enumerator_value)>)
constexpr auto enumerator = Enumerator<enumerator_name, enumerator_value> {};
}

namespace di::concepts {
template<typename T>
concept Enumerator = requires {
    { T::is_enumerator() } -> concepts::SameAs<bool>;
} && T::is_enumerator();
}

namespace di::reflection {
template<concepts::Enumerator... Es>
struct Enumerators : vocab::Tuple<Es...> {
    constexpr static bool is_fields() { return false; }
    constexpr static bool is_field() { return false; }
    constexpr static bool is_enumerator() { return false; }
    constexpr static bool is_enumerators() { return true; }
    constexpr static bool is_atom() { return false; }
    constexpr static bool is_integer() { return false; }
    constexpr static bool is_bool() { return false; }
    constexpr static bool is_string() { return false; }
    constexpr static bool is_list() { return false; }
    constexpr static bool is_map() { return false; }
};

namespace detail {
    struct MakeEnumeratorsFunction {
        template<concepts::Enumerator... Es>
        constexpr auto operator()(Es...) const {
            return Enumerators<Es...> {};
        }
    };
}

constexpr inline auto make_enumerators = detail::MakeEnumeratorsFunction {};
}
