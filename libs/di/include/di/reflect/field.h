#pragma once

#include "di/container/string/fixed_string.h"
#include "di/function/invoke.h"
#include "di/meta/core.h"
#include "di/meta/language.h"
#include "di/vocab/tuple/tuple.h"

namespace di::reflection {
template<container::FixedString field_name, auto field_pointer>
requires(concepts::MemberObjectPointer<decltype(field_pointer)>)
struct Field {
    constexpr static auto name = field_name;
    constexpr static auto pointer = field_pointer;

    using Object = meta::MemberPointerClass<decltype(pointer)>;
    using Type = meta::MemberPointerValue<decltype(pointer)>;

    template<typename T>
    requires(concepts::Invocable<decltype(pointer), T>)
    constexpr static auto get(T&& object) -> decltype(auto) {
        return function::invoke(pointer, util::forward<T>(object));
    }

    constexpr static auto is_fields() -> bool { return false; }
    constexpr static auto is_field() -> bool { return true; }
    constexpr static auto is_enumerator() -> bool { return false; }
    constexpr static auto is_enumerators() -> bool { return false; }
    constexpr static auto is_atom() -> bool { return false; }
    constexpr static auto is_integer() -> bool { return false; }
    constexpr static auto is_bool() -> bool { return false; }
    constexpr static auto is_string() -> bool { return false; }
    constexpr static auto is_list() -> bool { return false; }
    constexpr static auto is_map() -> bool { return false; }

    auto operator==(Field const&) const -> bool = default;
    auto operator<=>(Field const&) const = default;
};

template<container::FixedString field_name, auto field_pointer>
requires(concepts::MemberObjectPointer<decltype(field_pointer)>)
constexpr auto field = Field<field_name, field_pointer> {};
}

namespace di::concepts {
template<typename T>
concept Field = requires {
    { T::is_field() } -> concepts::SameAs<bool>;
} && T::is_field();
}

namespace di::reflection {
template<concepts::Field... Fs>
struct Fields : vocab::Tuple<Fs...> {
    constexpr static auto is_fields() -> bool { return true; }
    constexpr static auto is_field() -> bool { return false; }
    constexpr static auto is_enumerator() -> bool { return false; }
    constexpr static auto is_enumerators() -> bool { return false; }
    constexpr static auto is_atom() -> bool { return false; }
    constexpr static auto is_integer() -> bool { return false; }
    constexpr static auto is_bool() -> bool { return false; }
    constexpr static auto is_string() -> bool { return false; }
    constexpr static auto is_list() -> bool { return false; }
    constexpr static auto is_map() -> bool { return false; }
};

namespace detail {
    struct MakeFieldsFunction {
        template<concepts::Field... Fs>
        constexpr auto operator()(Fs...) const {
            return Fields<Fs...> {};
        }
    };
}

constexpr inline auto make_fields = detail::MakeFieldsFunction {};
}

namespace di {
using reflection::Field;
using reflection::field;
using reflection::Fields;
using reflection::make_fields;
}
