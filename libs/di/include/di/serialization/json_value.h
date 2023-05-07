#pragma once

#include <di/concepts/constructible_from.h>
#include <di/concepts/definitely_three_way_comparable_with.h>
#include <di/concepts/equality_comparable.h>
#include <di/concepts/three_way_comparable.h>
#include <di/container/algorithm/all_of.h>
#include <di/container/meta/container_value.h>
#include <di/container/string/string.h>
#include <di/container/string/string_view.h>
#include <di/container/tree/tree_map.h>
#include <di/container/vector/vector.h>
#include <di/function/tag_invoke.h>
#include <di/types/prelude.h>
#include <di/util/create.h>
#include <di/util/create_in_place.h>
#include <di/util/declval.h>
#include <di/vocab/expected/prelude.h>
#include <di/vocab/expected/try_infallible.h>
#include <di/vocab/optional/optional_forward_declaration.h>
#include <di/vocab/variant/holds_alternative.h>
#include <di/vocab/variant/variant.h>

namespace di::serialization::json {
class Value;

using Null = Void;
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

    constexpr bool is_null() const { return vocab::holds_alternative<Null>(*this); }
    constexpr bool is_boolean() const { return vocab::holds_alternative<Bool>(*this); }
    constexpr bool is_integer() const { return vocab::holds_alternative<Number>(*this); }
    constexpr bool is_number() const { return vocab::holds_alternative<Number>(*this); }
    constexpr bool is_string() const { return vocab::holds_alternative<String>(*this); }
    constexpr bool is_array() const { return vocab::holds_alternative<Array>(*this); }
    constexpr bool is_object() const { return vocab::holds_alternative<Object>(*this); }

    constexpr vocab::Optional<Bool> as_boolean() const {
        return vocab::get_if<Bool>(*this) % [](bool v) {
            return v;
        };
    }
    constexpr bool is_true() const { return as_boolean() == true; }
    constexpr bool is_false() const { return as_boolean() == false; }

    constexpr vocab::Optional<Number> as_integer() const { return vocab::get_if<Number>(*this); }
    constexpr vocab::Optional<Number> as_number() const { return vocab::get_if<Number>(*this); }

    constexpr vocab::Optional<String&> as_string() { return vocab::get_if<String>(*this); }
    constexpr vocab::Optional<String const&> as_string() const { return vocab::get_if<String>(*this); }
    constexpr vocab::Optional<Array&> as_array() { return vocab::get_if<Array>(*this); }
    constexpr vocab::Optional<Array const&> as_array() const { return vocab::get_if<Array>(*this); }
    constexpr vocab::Optional<Object&> as_object() { return vocab::get_if<Object>(*this); }
    constexpr vocab::Optional<Object const&> as_object() const { return vocab::get_if<Object>(*this); }

    // Object operator[] overloads.
    constexpr decltype(auto) operator[](String const& key) { return force_object()[key]; }
    constexpr decltype(auto) operator[](String&& key) { return force_object()[util::move(key)]; }

    template<typename U>
    requires(requires { util::declval<Object&>()[util::declval<U>()]; })
    constexpr decltype(auto) operator[](U&& key) {
        force_object();
        return force_object()[util::forward<U>(key)];
    }

    // Object at overloads.
    constexpr Optional<Value&> at(String const& key) {
        return as_object().and_then([&](auto& o) {
            return o.at(key);
        });
    }
    constexpr Optional<Value const&> at(String const& key) const {
        return as_object().and_then([&](auto const& o) {
            return o.at(key);
        });
    }

    template<typename U>
    requires(requires { util::declval<Object&>().at(util::declval<U&>()); })
    constexpr Optional<Value&> at(U&& key) {
        return as_object().and_then([&](auto& o) {
            return o.at(key);
        });
    }
    template<typename U>
    requires(requires { util::declval<Object const&>().at(util::declval<U&>()); })
    constexpr Optional<Value const&> at(U&& key) const {
        return as_object().and_then([&](auto const& o) {
            return o.at(key);
        });
    }

    // Object contains overloads.
    constexpr bool contains(String const& key) const {
        return as_object()
            .transform([&](auto const& o) {
                return o.contains(key);
            })
            .value_or(false);
    }
    template<typename U>
    constexpr bool contains(U&& key) const {
        return as_object()
            .transform([&](auto const& o) {
                return o.contains(util::forward<U>(key));
            })
            .value_or(false);
    }

    // Object erase overloads.
    constexpr size_t erase(String const& key) {
        return as_object()
            .transform([&](auto& o) {
                return o.erase(key);
            })
            .value_or(0);
    }
    template<typename U>
    requires(requires { util::declval<Object&>().erase(util::declval<U&>()); })
    constexpr size_t erase(U&& key) {
        return as_object()
            .transform([&](auto& o) {
                return o.erase(key);
            })
            .value_or(0);
    }

    // Object try_emplace overloads.
    template<typename... Args>
    requires(concepts::ConstructibleFrom<Value, Args...>)
    constexpr decltype(auto) try_emplace(String const& key, Args&&... args) {
        return force_object().try_emplace(key, util::forward<Args>(args)...);
    }
    template<typename... Args>
    requires(concepts::ConstructibleFrom<Value, Args...>)
    constexpr decltype(auto) try_emplace(String&& key, Args&&... args) {
        return force_object().try_emplace(util::move(key), util::forward<Args>(args)...);
    }
    template<typename U, typename... Args>
    requires(concepts::ConstructibleFrom<Value, Args...> &&
             requires { util::declval<Object&>().try_emplace(util::declval<U>(), util::declval<Args>()...); })
    constexpr decltype(auto) try_emplace(U&& key, Args&&... args) {
        return force_object().try_emplace(util::forward<U>(key), util::forward<Args>(args)...);
    }

    // Object insert_or_assign overloads.
    template<typename V>
    requires(concepts::ConstructibleFrom<Value, V>)
    constexpr decltype(auto) insert_or_assign(String const& key, V&& value) {
        return force_object().insert_or_assign(key, util::forward<V>(value));
    }
    template<typename V>
    requires(concepts::ConstructibleFrom<Value, V>)
    constexpr decltype(auto) insert_or_assign(String&& key, V&& value) {
        return force_object().insert_or_assign(util::move(key), util::forward<V>(value));
    }
    template<typename U, typename V>
    requires(concepts::ConstructibleFrom<Value, V> &&
             requires { util::declval<Object&>().insert_or_assign(util::declval<U>(), util::declval<Value>()); })
    constexpr decltype(auto) insert_or_assign(U&& key, V&& value) {
        return force_object().insert_or_assign(util::forward<U>(key), util::forward<V>(value));
    }

    constexpr bool empty() const {
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
    constexpr usize size() const {
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
    constexpr Optional<Value&> front() {
        return vocab::visit(function::overload(
                                [](Array& v) {
                                    return v.front();
                                },
                                [](auto&) {
                                    return Optional<Value&> {};
                                }),
                            *this);
    }
    constexpr Optional<Value const&> front() const {
        return vocab::visit(function::overload(
                                [](Array const& v) {
                                    return v.front();
                                },
                                [](auto const&) {
                                    return Optional<Value const&> {};
                                }),
                            *this);
    }

    constexpr Optional<Value&> back() {
        return vocab::visit(function::overload(
                                [](Array& v) {
                                    return v.back();
                                },
                                [](auto&) {
                                    return Optional<Value&> {};
                                }),
                            *this);
    }
    constexpr Optional<Value const&> back() const {
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
    constexpr Value& operator[](usize index) { return as_array().value()[index]; }
    constexpr Value const& operator[](usize index) const { return as_array().value()[index]; }

    constexpr Optional<Value&> at(usize index) {
        return as_array().and_then([&](auto& a) {
            return a.at(index);
        });
    }
    constexpr Optional<Value const&> at(usize index) const {
        return as_array().and_then([&](auto const& a) {
            return a.at(index);
        });
    }

    // Array push_front/pop_back overloads.
    constexpr decltype(auto) push_back(Value&& value) { return force_array().push_back(util::move(value)); }
    constexpr Optional<Value> pop_back() {
        return as_array().and_then([](auto& a) {
            return a.pop_back();
        });
    }

private:
    constexpr friend bool operator==(Value const& a, container::StringView view) { return a.as_string() == view; }
    constexpr friend auto operator<=>(Value const& a, container::StringView view) {
        constexpr auto string_index = usize(3);
        if (auto result = a.index() <=> string_index; result != 0) {
            return result;
        }
        return *a.as_string() <=> view;
    }

    constexpr Array& force_array() {
        if (!is_array()) {
            *this = Array {};
        }
        return as_array().value();
    }

    constexpr Object& force_object() {
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
