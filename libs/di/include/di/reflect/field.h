#pragma once

#include <di/concepts/instance_of.h>
#include <di/concepts/same_as.h>
#include <di/container/string/fixed_string.h>
#include <di/function/invoke.h>
#include <di/meta/member_pointer_class.h>
#include <di/meta/member_pointer_value.h>
#include <di/vocab/tuple/tuple.h>

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
    constexpr static decltype(auto) get(T&& object) {
        return function::invoke(pointer, util::forward<T>(object));
    }

    constexpr static bool is_fields() { return false; }
    constexpr static bool is_field() { return true; }
    constexpr static bool is_enumerator() { return false; }
    constexpr static bool is_enumerators() { return false; }
    constexpr static bool is_atom() { return false; }
    constexpr static bool is_integer() { return false; }
    constexpr static bool is_bool() { return false; }
    constexpr static bool is_string() { return false; }
    constexpr static bool is_list() { return false; }
    constexpr static bool is_map() { return false; }

    bool operator==(Field const&) const = default;
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
    constexpr static bool is_fields() { return true; }
    constexpr static bool is_field() { return false; }
    constexpr static bool is_enumerator() { return false; }
    constexpr static bool is_enumerators() { return false; }
    constexpr static bool is_atom() { return false; }
    constexpr static bool is_integer() { return false; }
    constexpr static bool is_bool() { return false; }
    constexpr static bool is_string() { return false; }
    constexpr static bool is_list() { return false; }
    constexpr static bool is_map() { return false; }
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
