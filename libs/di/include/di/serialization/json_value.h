#pragma once

#include "di/container/algorithm/all_of.h"
#include "di/container/meta/container_value.h"
#include "di/container/string/string.h"
#include "di/container/string/string_view.h"
#include "di/container/tree/tree_map.h"
#include "di/container/vector/vector.h"
#include "di/format/formatter.h"
#include "di/function/tag_invoke.h"
#include "di/io/interface/writer.h"
#include "di/meta/compare.h"
#include "di/meta/operations.h"
#include "di/serialization/json_serializer.h"
#include "di/serialization/serialize.h"
#include "di/types/prelude.h"
#include "di/util/create.h"
#include "di/util/create_in_place.h"
#include "di/util/declval.h"
#include "di/vocab/expected/prelude.h"
#include "di/vocab/expected/try_infallible.h"
#include "di/vocab/optional/optional_forward_declaration.h"
#include "di/vocab/variant/holds_alternative.h"
#include "di/vocab/variant/variant.h"

namespace di::serialization::json {
class Value;

struct Null {
    explicit Null() = default;

    template<concepts::Encoding Enc>
    constexpr friend auto tag_invoke(types::Tag<format::formatter_in_place>, InPlaceType<Null>,
                                     format::FormatParseContext<Enc>& parse_context, bool debug) {
        return format::formatter<container::StringView>(parse_context, debug) %
               [](concepts::CopyConstructible auto formatter) {
                   return [=](concepts::FormatContext auto& context, Null) {
                       return formatter(context, "null"_sv);
                   };
               };
    }

    constexpr friend auto tag_invoke(types::Tag<serialization::serialize>, JsonFormat, auto& serializer, Null) {
        return serializer.serialize_null();
    }

    auto operator==(Null const&) const -> bool = default;
    auto operator<=>(Null const&) const = default;
};

constexpr inline auto null = Null {};

using Bool = bool;

// NOTE: this should support floating point in the future.
using Number = i64;

using String = container::String;
using Array = container::Vector<Value>;
using Object = container::TreeMap<container::String, Value>;
using KeyValue = vocab::Tuple<String, Value>;
}

namespace di::concepts::detail {
template<>
struct DefinitelyEqualityComparableWith<serialization::json::Value, serialization::json::Value> {
    constexpr static bool value = true;
};

template<>
struct DefinitelyEqualityComparableWith<serialization::json::Array, serialization::json::Array> {
    constexpr static bool value = true;
};

template<>
struct DefinitelyEqualityComparableWith<serialization::json::Object, serialization::json::Object> {
    constexpr static bool value = true;
};

template<>
struct DefinitelyEqualityComparableWith<serialization::json::KeyValue, serialization::json::KeyValue> {
    constexpr static bool value = true;
};

template<>
struct DefinitelyThreeWayComparableWith<serialization::json::Value, serialization::json::Value> {
    using Type = di::strong_ordering;
};

template<>
struct DefinitelyThreeWayComparableWith<serialization::json::Array, serialization::json::Array> {
    using Type = di::strong_ordering;
};

template<>
struct DefinitelyThreeWayComparableWith<serialization::json::Object, serialization::json::Object> {
    using Type = di::strong_ordering;
};

template<>
struct DefinitelyThreeWayComparableWith<serialization::json::KeyValue, serialization::json::KeyValue> {
    using Type = di::strong_ordering;
};
}

namespace di::serialization::json {
class Value : public vocab::Variant<Null, Bool, Number, String, Array, Object> {
    using Base = vocab::Variant<Null, Bool, Number, String, Array, Object>;

    constexpr static usize alternatives = 6;

    template<concepts::SameAs<types::Tag<util::create_in_place>> Tag = types::Tag<util::create_in_place>,
             typename... Args>
    requires(concepts::CreatableFrom<String, Args...>)
    constexpr friend auto tag_invoke(Tag, InPlaceType<Value>, Args&&... args) {
        return as_fallible(util::create<String>(util::forward<Args>(args)...)) % [](String&& v) {
            return Value { util::move(v) };
        } | try_infallible;
    }

public:
    using Base::Base;
    using Base::operator=;

    constexpr auto is_null() const -> bool { return vocab::holds_alternative<Null>(*this); }
    constexpr auto is_boolean() const -> bool { return vocab::holds_alternative<Bool>(*this); }
    constexpr auto is_integer() const -> bool { return vocab::holds_alternative<Number>(*this); }
    constexpr auto is_number() const -> bool { return vocab::holds_alternative<Number>(*this); }
    constexpr auto is_string() const -> bool { return vocab::holds_alternative<String>(*this); }
    constexpr auto is_array() const -> bool { return vocab::holds_alternative<Array>(*this); }
    constexpr auto is_object() const -> bool { return vocab::holds_alternative<Object>(*this); }

    constexpr auto as_boolean() const -> vocab::Optional<Bool> {
        return vocab::get_if<Bool>(*this) % [](bool v) {
            return v;
        };
    }
    constexpr auto is_true() const -> bool { return as_boolean() == true; }
    constexpr auto is_false() const -> bool { return as_boolean() == false; }

    constexpr auto as_integer() const -> vocab::Optional<Number> { return vocab::get_if<Number>(*this); }
    constexpr auto as_number() const -> vocab::Optional<Number> { return vocab::get_if<Number>(*this); }

    constexpr auto as_string() -> vocab::Optional<String&> { return vocab::get_if<String>(*this); }
    constexpr auto as_string() const -> vocab::Optional<String const&> { return vocab::get_if<String>(*this); }
    constexpr auto as_array() -> vocab::Optional<Array&> { return vocab::get_if<Array>(*this); }
    constexpr auto as_array() const -> vocab::Optional<Array const&> { return vocab::get_if<Array>(*this); }
    constexpr auto as_object() -> vocab::Optional<Object&> { return vocab::get_if<Object>(*this); }
    constexpr auto as_object() const -> vocab::Optional<Object const&> { return vocab::get_if<Object>(*this); }

    // Object operator[] overloads.
    constexpr auto operator[](String const& key) -> decltype(auto) { return force_object()[key]; }
    constexpr auto operator[](String&& key) -> decltype(auto) { return force_object()[util::move(key)]; }

    template<typename U>
    requires(requires { util::declval<Object&>()[util::declval<U>()]; })
    constexpr auto operator[](U&& key) -> decltype(auto) {
        force_object();
        return force_object()[util::forward<U>(key)];
    }

    // Object at overloads.
    constexpr auto at(String const& key) -> Optional<Value&> {
        return as_object().and_then([&](auto& o) {
            return o.at(key);
        });
    }
    constexpr auto at(String const& key) const -> Optional<Value const&> {
        return as_object().and_then([&](auto const& o) {
            return o.at(key);
        });
    }

    template<typename U>
    requires(requires { util::declval<Object&>().at(util::declval<U&>()); })
    constexpr auto at(U&& key) -> Optional<Value&> {
        return as_object().and_then([&](auto& o) {
            return o.at(key);
        });
    }
    template<typename U>
    requires(requires { util::declval<Object const&>().at(util::declval<U&>()); })
    constexpr auto at(U&& key) const -> Optional<Value const&> {
        return as_object().and_then([&](auto const& o) {
            return o.at(key);
        });
    }

    // Object contains overloads.
    constexpr auto contains(String const& key) const -> bool {
        return as_object()
            .transform([&](auto const& o) {
                return o.contains(key);
            })
            .value_or(false);
    }
    template<typename U>
    constexpr auto contains(U&& key) const -> bool {
        return as_object()
            .transform([&](auto const& o) {
                return o.contains(util::forward<U>(key));
            })
            .value_or(false);
    }

    // Object erase overloads.
    constexpr auto erase(String const& key) -> size_t {
        return as_object()
            .transform([&](auto& o) {
                return o.erase(key);
            })
            .value_or(0);
    }
    template<typename U>
    requires(requires { util::declval<Object&>().erase(util::declval<U&>()); })
    constexpr auto erase(U&& key) -> size_t {
        return as_object()
            .transform([&](auto& o) {
                return o.erase(key);
            })
            .value_or(0);
    }

    // Object try_emplace overloads.
    template<typename... Args>
    requires(concepts::ConstructibleFrom<Value, Args...>)
    constexpr auto try_emplace(String const& key, Args&&... args) -> decltype(auto) {
        return force_object().try_emplace(key, util::forward<Args>(args)...);
    }
    template<typename... Args>
    requires(concepts::ConstructibleFrom<Value, Args...>)
    constexpr auto try_emplace(String&& key, Args&&... args) -> decltype(auto) {
        return force_object().try_emplace(util::move(key), util::forward<Args>(args)...);
    }
    template<typename U, typename... Args>
    requires(concepts::ConstructibleFrom<Value, Args...> &&
             requires { util::declval<Object&>().try_emplace(util::declval<U>(), util::declval<Args>()...); })
    constexpr auto try_emplace(U&& key, Args&&... args) -> decltype(auto) {
        return force_object().try_emplace(util::forward<U>(key), util::forward<Args>(args)...);
    }

    // Object insert_or_assign overloads.
    template<typename V>
    requires(concepts::ConstructibleFrom<Value, V>)
    constexpr auto insert_or_assign(String const& key, V&& value) -> decltype(auto) {
        return force_object().insert_or_assign(key, util::forward<V>(value));
    }
    template<typename V>
    requires(concepts::ConstructibleFrom<Value, V>)
    constexpr auto insert_or_assign(String&& key, V&& value) -> decltype(auto) {
        return force_object().insert_or_assign(util::move(key), util::forward<V>(value));
    }
    template<typename U, typename V>
    requires(concepts::ConstructibleFrom<Value, V> &&
             requires { util::declval<Object&>().insert_or_assign(util::declval<U>(), util::declval<Value>()); })
    constexpr auto insert_or_assign(U&& key, V&& value) -> decltype(auto) {
        return force_object().insert_or_assign(util::forward<U>(key), util::forward<V>(value));
    }

    constexpr auto empty() const -> bool {
        return vocab::visit(function::overload(
                                [](Object const& v) {
                                    return v.empty();
                                },
                                [](Array const& v) {
                                    return v.empty();
                                },
                                [](auto const&) {
                                    return true;
                                }),
                            *this);
    }
    constexpr auto size() const -> usize {
        return vocab::visit(function::overload(
                                [](Object const& v) {
                                    return v.size();
                                },
                                [](Array const& v) {
                                    return v.size();
                                },
                                [](auto const&) {
                                    return usize(0);
                                }),
                            *this);
    }

    constexpr void clear() {
        vocab::visit(function::overload(
                         [](Object& v) {
                             v.clear();
                         },
                         [](Array& v) {
                             v.clear();
                         },
                         [](auto&) {}),
                     *this);
    }

    // Array front/back overloads.
    constexpr auto front() -> Optional<Value&> {
        return vocab::visit(function::overload(
                                [](Array& v) {
                                    return v.front();
                                },
                                [](auto&) {
                                    return Optional<Value&> {};
                                }),
                            *this);
    }
    constexpr auto front() const -> Optional<Value const&> {
        return vocab::visit(function::overload(
                                [](Array const& v) {
                                    return v.front();
                                },
                                [](auto const&) {
                                    return Optional<Value const&> {};
                                }),
                            *this);
    }

    constexpr auto back() -> Optional<Value&> {
        return vocab::visit(function::overload(
                                [](Array& v) {
                                    return v.back();
                                },
                                [](auto&) {
                                    return Optional<Value&> {};
                                }),
                            *this);
    }
    constexpr auto back() const -> Optional<Value const&> {
        return vocab::visit(function::overload(
                                [](Array const& v) {
                                    return v.back();
                                },
                                [](auto const&) {
                                    return Optional<Value const&> {};
                                }),
                            *this);
    }

    // Array operator[]/at overloads.
    constexpr auto operator[](usize index) -> Value& { return as_array().value()[index]; }
    constexpr auto operator[](usize index) const -> Value const& { return as_array().value()[index]; }

    constexpr auto at(usize index) -> Optional<Value&> {
        return as_array().and_then([&](auto& a) {
            return a.at(index);
        });
    }
    constexpr auto at(usize index) const -> Optional<Value const&> {
        return as_array().and_then([&](auto const& a) {
            return a.at(index);
        });
    }

    // Array push_front/pop_back overloads.
    constexpr auto push_back(Value&& value) -> decltype(auto) { return force_array().push_back(util::move(value)); }
    constexpr auto pop_back() -> Optional<Value> {
        return as_array().and_then([](auto& a) {
            return a.pop_back();
        });
    }

private:
    template<typename S>
    constexpr friend auto tag_invoke(types::Tag<serialize>, JsonFormat, S& serializer, Value const& value)
        -> meta::SerializeResult<S> {
        return vocab::visit(
            [&](auto const& x) {
                return serialize(serializer, x);
            },
            value);
    }

    constexpr friend auto tag_invoke(types::Tag<serializable>, JsonFormat, InPlaceType<Value>) -> bool { return true; }

    template<concepts::Encoding Enc, concepts::SameAs<InPlaceType<Value>> X = InPlaceType<Value>,
             concepts::SameAs<bool> B = bool,
             concepts::SameAs<types::Tag<format::formatter_in_place>> Tag = types::Tag<format::formatter_in_place>>
    constexpr friend auto tag_invoke(Tag, X, format::FormatParseContext<Enc>& parse_context, B debug) {
        return format::formatter<container::StringView>(parse_context, debug) %
               [](concepts::CopyConstructible auto formatter) {
                   return [=](concepts::FormatContext auto& context, Value const& value) {
                       auto string = serialization::to_json_string(value, JsonSerializerConfig().pretty());
                       if (!string) {
                           return formatter(context, "[<JSON serializer error>]"_sv);
                       }
                       return formatter(context, string->view());
                   };
               };
    }

    constexpr friend auto operator==(Value const& a, container::StringView view) -> bool {
        return a.as_string() == view;
    }
    constexpr friend auto operator<=>(Value const& a, container::StringView view) {
        constexpr auto string_index = usize(3);
        if (auto result = a.index() <=> string_index; result != 0) {
            return result;
        }
        return *a.as_string() <=> view;
    }

    constexpr auto force_array() -> Array& {
        if (!is_array()) {
            *this = Array {};
        }
        return as_array().value();
    }

    constexpr auto force_object() -> Object& {
        if (!is_object()) {
            *this = Object {};
        }
        return as_object().value();
    }
};
}

namespace di {
namespace json = serialization::json;
}
