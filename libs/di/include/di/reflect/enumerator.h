#pragma once

#include "di/container/string/fixed_string.h"
#include "di/meta/core.h"
#include "di/meta/language.h"
#include "di/vocab/tuple/tuple.h"

namespace di::reflection {
template<container::FixedString enumerator_name, auto enumerator_value>
requires(concepts::Enum<decltype(enumerator_value)>)
struct Enumerator {
    constexpr static auto name = enumerator_name;
    constexpr static auto value = enumerator_value;

    using Type = decltype(enumerator_value);

    constexpr static auto get() -> Type { return value; }

    constexpr static auto is_fields() -> bool { return false; }
    constexpr static auto is_field() -> bool { return false; }
    constexpr static auto is_enumerator() -> bool { return true; }
    constexpr static auto is_enumerators() -> bool { return false; }
    constexpr static auto is_atom() -> bool { return false; }
    constexpr static auto is_integer() -> bool { return false; }
    constexpr static auto is_bool() -> bool { return false; }
    constexpr static auto is_string() -> bool { return false; }
    constexpr static auto is_list() -> bool { return false; }
    constexpr static auto is_map() -> bool { return false; }

    auto operator==(Enumerator const&) const -> bool = default;
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
template<concepts::Constexpr EnumName, concepts::Enumerator... Es>
struct Enumerators : vocab::Tuple<Es...> {
    constexpr static auto name = EnumName::value;

    constexpr static auto is_fields() -> bool { return false; }
    constexpr static auto is_field() -> bool { return false; }
    constexpr static auto is_enumerator() -> bool { return false; }
    constexpr static auto is_enumerators() -> bool { return true; }
    constexpr static auto is_atom() -> bool { return false; }
    constexpr static auto is_integer() -> bool { return false; }
    constexpr static auto is_bool() -> bool { return false; }
    constexpr static auto is_string() -> bool { return false; }
    constexpr static auto is_list() -> bool { return false; }
    constexpr static auto is_map() -> bool { return false; }
};

namespace detail {
    template<container::FixedString enum_name>
    struct MakeEnumeratorsFunction {
        template<concepts::Enumerator... Es>
        constexpr auto operator()(Es...) const {
            return Enumerators<meta::Constexpr<enum_name>, Es...> {};
        }
    };
}

template<container::FixedString enum_name>
constexpr inline auto make_enumerators = detail::MakeEnumeratorsFunction<enum_name> {};
}

namespace di {
using reflection::Enumerator;
using reflection::enumerator;
using reflection::Enumerators;
using reflection::make_enumerators;
}
