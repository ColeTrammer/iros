#pragma once

#include <di/any/concepts/impl.h>
#include <di/container/string/encoding.h>
#include <di/container/string/fixed_string.h>
#include <di/container/string/fixed_string_to_utf8_string_view.h>
#include <di/container/string/string_view.h>
#include <di/io/interface/reader.h>
#include <di/io/prelude.h>
#include <di/io/string_reader.h>
#include <di/meta/core.h>
#include <di/meta/operations.h>
#include <di/platform/compiler.h>
#include <di/platform/prelude.h>
#include <di/serialization/deserialize.h>
#include <di/serialization/deserialize_string.h>
#include <di/serialization/json_serializer.h>
#include <di/serialization/json_value.h>
#include <di/types/in_place_type.h>
#include <di/types/prelude.h>
#include <di/util/exchange.h>
#include <di/util/reference_wrapper.h>
#include <di/util/to_underlying.h>
#include <di/vocab/optional/nullopt.h>
#include <di/vocab/optional/optional_forward_declaration.h>
#include <di/vocab/tuple/tuple_for_each.h>
#include <di/vocab/tuple/tuple_sequence.h>

namespace di::serialization {
/// @brief A deserializer for the JSON format.
///
/// @tparam Reader The type of the reader to read from.
///
/// This implements the JSON grammar as specified in [RFC 8259](https://www.rfc-editor.org/rfc/rfc8259).
template<concepts::Impl<io::Reader> Reader>
class JsonDeserializer {
private:
    template<typename T>
    using Result = meta::ReaderResult<T, Reader>;

public:
    using DeserializationFormat = JsonFormat;

    template<typename T>
    requires(concepts::ConstructibleFrom<Reader, T>)
    constexpr explicit JsonDeserializer(T&& reader) : m_reader(util::forward<T>(reader)) {}

    constexpr Result<json::Value> deserialize(InPlaceType<json::Value>) {
        auto result = DI_TRY(deserialize_value());
        DI_TRY(skip_whitespace());
        return result;
    }

    template<typename T, concepts::InstanceOf<reflection::Fields> M>
    constexpr Result<T> deserialize(InPlaceType<T>, M fields) {
        // NOTE: for now, this requires T be default constructible.
        auto result = T {};

        DI_TRY(skip_whitespace());
        DI_TRY(expect('{'));

        auto first = true;
        for (;;) {
            DI_TRY(skip_whitespace());
            auto code_point = DI_TRY(peek_next_code_point());
            if (!code_point) {
                return vocab::Unexpected(BasicError::InvalidArgument);
            }
            if (*code_point == U'}') {
                break;
            }
            if (!util::exchange(first, false)) {
                DI_TRY(expect(U','));
            }
            auto key = DI_TRY(deserialize_string());
            DI_TRY(skip_whitespace());
            DI_TRY(expect(U':'));
            DI_TRY(skip_whitespace());

            auto found = false;
            DI_TRY(vocab::tuple_sequence<Result<void>>(
                [&](auto field) -> Result<void> {
                    if (key == container::fixed_string_to_utf8_string_view<field.name>()) {
                        using Value = meta::Type<decltype(field)>;
                        if constexpr (concepts::Optional<Value>) {
                            field.get(result) = DI_TRY(serialization::deserialize<meta::OptionalValue<Value>>(*this));
                        } else {
                            field.get(result) = DI_TRY(serialization::deserialize<Value>(*this));
                        }
                        found = true;
                    }
                    return {};
                },
                fields));
            if (!found) {
                return vocab::Unexpected(BasicError::InvalidArgument);
            }
        }

        DI_TRY(expect('}'));
        DI_TRY(skip_whitespace());
        return result;
    }

    template<typename T, concepts::InstanceOf<reflection::Enumerators> M>
    constexpr Result<T> deserialize(InPlaceType<T>, M enumerators) {
        DI_TRY(skip_whitespace());
        auto string = DI_TRY(deserialize_string());
        DI_TRY(skip_whitespace());
        auto result = T(0);
        auto found = false;

        vocab::tuple_for_each(
            [&](auto enumerator) {
                if (string == container::fixed_string_to_utf8_string_view<enumerator.name>()) {
                    result = T(enumerator.value);
                    found = true;
                }
            },
            enumerators);

        if (!found) {
            return vocab::Unexpected(BasicError::InvalidArgument);
        }
        return result;
    }

    template<typename T, concepts::InstanceOf<reflection::Atom> M>
    requires(M::is_bool() || M::is_integer() || M::is_string())
    constexpr Result<T> deserialize(InPlaceType<T>, M) {
        if constexpr (M::is_bool()) {
            auto result = DI_TRY(deserialize_bool());
            DI_TRY(skip_whitespace());
            return result;
        } else if constexpr (M::is_integer()) {
            auto result = DI_TRY(deserialize_number(in_place_type<T>));
            DI_TRY(skip_whitespace());
            return result;
        } else if constexpr (M::is_string()) {
            auto result = DI_TRY(deserialize_string());
            DI_TRY(skip_whitespace());
            return result;
        }
    }

    template<typename T, concepts::InstanceOf<reflection::Atom> M>
    requires(M::is_list() && concepts::Deserializable<meta::ContainerValue<T>, JsonDeserializer>)
    constexpr Result<T> deserialize(InPlaceType<T>, M) {
        auto result = T {};

        DI_TRY(skip_whitespace());
        DI_TRY(expect('['));

        for (;;) {
            DI_TRY(skip_whitespace());
            auto code_point = DI_TRY(peek_next_code_point());
            if (!code_point) {
                return vocab::Unexpected(BasicError::InvalidArgument);
            }
            if (*code_point == U']') {
                break;
            }
            if (!result.empty()) {
                DI_TRY(expect(U','));
            }
            result.push_back(DI_TRY(serialization::deserialize<meta::ContainerValue<T>>(*this)));
        }

        DI_TRY(expect(']'));
        DI_TRY(skip_whitespace());
        return result;
    }

    template<typename T, concepts::InstanceOf<reflection::Atom> M>
    requires(M::is_map() && concepts::SameAs<json::String, meta::TupleElement<meta::ContainerValue<T>, 0>> &&
             concepts::Deserializable<meta::TupleElement<meta::ContainerValue<T>, 1>, JsonDeserializer>)
    constexpr Result<T> deserialize(InPlaceType<T>, M) {
        auto result = T {};

        DI_TRY(skip_whitespace());
        DI_TRY(expect('{'));

        for (;;) {
            DI_TRY(skip_whitespace());
            auto code_point = DI_TRY(peek_next_code_point());
            if (!code_point) {
                return vocab::Unexpected(BasicError::InvalidArgument);
            }
            if (*code_point == U'}') {
                break;
            }
            if (!result.empty()) {
                DI_TRY(expect(U','));
            }
            auto key = DI_TRY(deserialize_string());
            DI_TRY(skip_whitespace());
            DI_TRY(expect(U':'));
            DI_TRY(skip_whitespace());
            auto value = DI_TRY(serialization::deserialize<meta::TupleElement<meta::ContainerValue<T>, 1>>(*this));
            result.insert_or_assign(util::move(key), util::move(value));
        }

        DI_TRY(expect('}'));
        DI_TRY(skip_whitespace());
        return result;
    }

    constexpr Result<json::Null> deserialize(InPlaceType<json::Null>) {
        auto result = DI_TRY(deserialize_null());
        DI_TRY(skip_whitespace());
        return result;
    }

    constexpr Reader& reader() & { return m_reader; }
    constexpr Reader const& reader() const& { return m_reader; }
    constexpr Reader&& reader() && { return util::move(*this).m_reader; }

private:
    constexpr static bool is_whitespace(c32 code_point) {
        return code_point == ' ' || code_point == '\t' || code_point == '\n' || code_point == '\r';
    }

    constexpr Result<void> expect(c32 expected) {
        auto code_point = DI_TRY(next_code_point());
        if (!code_point || *code_point != expected) {
            return vocab::Unexpected(BasicError::InvalidArgument);
        }
        return {};
    }

    constexpr Result<void> fill_next_code_point() {
        // FIXME: handle UTF-8.
        auto byte = vocab::Array<types::byte, 1> {};
        auto nread = DI_TRY(io::read_some(m_reader, byte));
        if (nread == 0) {
            m_at_end = true;
        }
        m_next_code_point = c32(byte[0]);
        return {};
    }

    constexpr Result<vocab::Optional<c32>> peek_next_code_point() {
        if (m_at_end) {
            return vocab::nullopt;
        }
        if (!m_next_code_point) {
            DI_TRY(fill_next_code_point());
            if (m_at_end) {
                return vocab::nullopt;
            }
        }
        return *m_next_code_point;
    }

    constexpr void consume() { m_next_code_point = vocab::nullopt; }

    constexpr Result<vocab::Optional<c32>> next_code_point() {
        if (m_at_end) {
            return vocab::nullopt;
        }
        if (!m_next_code_point) {
            DI_TRY(fill_next_code_point());
            if (m_at_end) {
                return vocab::nullopt;
            }
        }
        return *util::exchange(m_next_code_point, vocab::nullopt);
    }

    constexpr Result<c32> require_next_code_point() {
        auto code_point = DI_TRY(next_code_point());
        if (!code_point) {
            return vocab::Unexpected(BasicError::InvalidArgument);
        }
        return *code_point;
    }

    constexpr Result<void> skip_whitespace() {
        for (;;) {
            auto code_point = DI_TRY(peek_next_code_point());
            if (!code_point || !is_whitespace(*code_point)) {
                return {};
            }
            consume();
        }
    }

    constexpr Result<json::Value> deserialize_value() {
        DI_TRY(skip_whitespace());

        auto code_point = DI_TRY(peek_next_code_point());
        if (!code_point) {
            return vocab::Unexpected(BasicError::InvalidArgument);
        }

        switch (*code_point) {
            case U'n':
                return deserialize_null();
            case U't':
                return deserialize_true();
            case U'f':
                return deserialize_false();
            case U'"':
                return deserialize_string();
            case U'-':
            case U'0':
            case U'1':
            case U'2':
            case U'3':
            case U'4':
            case U'5':
            case U'6':
            case U'7':
            case U'8':
            case U'9':
                return deserialize_number(in_place_type<json::Number>);
            case U'{':
                return deserialize_object();
            case U'[':
                return deserialize_array();
            default:
                return vocab::Unexpected(BasicError::InvalidArgument);
        }
    }

    constexpr Result<json::Null> deserialize_null() {
        DI_TRY(skip_whitespace());

        DI_TRY(expect(U'n'));
        DI_TRY(expect(U'u'));
        DI_TRY(expect(U'l'));
        DI_TRY(expect(U'l'));
        return json::null;
    }

    constexpr Result<json::Bool> deserialize_bool() {
        DI_TRY(skip_whitespace());

        auto code_point = DI_TRY(require_next_code_point());
        switch (code_point) {
            case U't':
                DI_TRY(expect(U'r'));
                DI_TRY(expect(U'u'));
                DI_TRY(expect(U'e'));
                return true;
            case U'f':
                DI_TRY(expect(U'a'));
                DI_TRY(expect(U'l'));
                DI_TRY(expect(U's'));
                DI_TRY(expect(U'e'));
                return false;
            default:
                return vocab::Unexpected(BasicError::InvalidArgument);
        }
    }

    constexpr Result<json::Bool> deserialize_true() {
        DI_TRY(skip_whitespace());
        DI_TRY(expect(U't'));
        DI_TRY(expect(U'r'));
        DI_TRY(expect(U'u'));
        DI_TRY(expect(U'e'));
        return true;
    }

    constexpr Result<json::Bool> deserialize_false() {
        DI_TRY(skip_whitespace());
        DI_TRY(expect(U'f'));
        DI_TRY(expect(U'a'));
        DI_TRY(expect(U'l'));
        DI_TRY(expect(U's'));
        DI_TRY(expect(U'e'));
        return false;
    }

    constexpr Result<json::String> deserialize_string() {
        DI_TRY(skip_whitespace());
        DI_TRY(expect(U'"'));

        auto string = json::String {};
        for (;;) {
            auto code_point = DI_TRY(next_code_point());
            if (!code_point || *code_point < 0x20) {
                return vocab::Unexpected(BasicError::InvalidArgument);
            }
            if (*code_point == U'"') {
                break;
            }
            // FIXME: handle escape sequences.
            string.push_back(*code_point);
        }
        return string;
    }

    template<concepts::Integer T>
    constexpr Result<json::Number> deserialize_number(InPlaceType<T>) {
        DI_TRY(skip_whitespace());
        auto first_code_point = DI_TRY(require_next_code_point());

        auto string = json::String {};
        if (first_code_point == U'-') {
            string.push_back(first_code_point);
            first_code_point = DI_TRY(require_next_code_point());
            if (first_code_point < U'0' || first_code_point > U'9') {
                return vocab::Unexpected(BasicError::InvalidArgument);
            }
        }
        if (first_code_point == U'0') {
            return 0;
        }

        string.push_back(first_code_point);

        for (;;) {
            auto code_point = DI_TRY(peek_next_code_point());
            if (!code_point) {
                break;
            }

            if (*code_point < U'0' || *code_point > U'9') {
                break;
            }

            string.push_back(*code_point);
            consume();
        }

        // FIXME: handle decimal point and exponent for floating point numbers.
        auto result = parser::parse<T>(string);
        if (!result) {
            return vocab::Unexpected(BasicError::InvalidArgument);
        }
        return *result;
    }

    constexpr Result<json::Array> deserialize_array() {
        DI_TRY(skip_whitespace());
        DI_TRY(expect(U'['));

        auto array = json::Array {};
        for (;;) {
            DI_TRY(skip_whitespace());
            auto code_point = DI_TRY(peek_next_code_point());
            if (!code_point) {
                return vocab::Unexpected(BasicError::InvalidArgument);
            }
            if (*code_point == U']') {
                break;
            }
            if (!array.empty()) {
                DI_TRY(expect(U','));
            }
            array.push_back(DI_TRY(deserialize_value()));
        }

        DI_TRY(expect(U']'));
        return array;
    }

    constexpr Result<json::Object> deserialize_object() {
        DI_TRY(skip_whitespace());
        DI_TRY(expect(U'{'));

        auto object = json::Object {};
        for (;;) {
            DI_TRY(skip_whitespace());
            auto code_point = DI_TRY(peek_next_code_point());
            if (!code_point) {
                return vocab::Unexpected(BasicError::InvalidArgument);
            }
            if (*code_point == U'}') {
                break;
            }
            if (!object.empty()) {
                DI_TRY(expect(U','));
                DI_TRY(skip_whitespace());
            }
            auto key = DI_TRY(deserialize_string());
            DI_TRY(skip_whitespace());
            DI_TRY(expect(U':'));
            auto value = DI_TRY(deserialize_value());
            object.insert_or_assign(util::move(key), util::move(value));
        }

        DI_TRY(expect(U'}'));
        return object;
    }

    Reader m_reader;
    vocab::Optional<c32> m_next_code_point;
    bool m_at_end { false };
};

template<typename T>
JsonDeserializer(T&&) -> JsonDeserializer<T>;

namespace detail {
    template<typename T>
    struct FromJsonStringFunction {
        template<typename... Args>
        requires(concepts::ConstructibleFrom<JsonDeserializer<StringReader<container::StringView>>,
                                             StringReader<container::StringView>, Args...>)
        constexpr auto operator()(container::StringView view, Args&&... args) const {
            return serialization::deserialize_string<T>(json_format, view, util::forward<Args>(args)...);
        }
    };
}

template<concepts::Deserializable<JsonDeserializer<StringReader<container::StringView>>> T = json::Value>
constexpr inline auto from_json_string = detail::FromJsonStringFunction<T> {};

namespace detail {
    template<typename T>
    struct DeserializeJsonFunction {
        template<concepts::Impl<io::Reader> Reader, typename... Args>
        requires(concepts::ConstructibleFrom<JsonDeserializer<ReferenceWrapper<meta::RemoveCVRef<Reader>>>, Reader&,
                                             Args...> &&
                 concepts::Deserializable<T, JsonDeserializer<ReferenceWrapper<meta::RemoveCVRef<Reader>>>>)
        constexpr auto operator()(Reader&& reader, Args&&... args) const {
            return serialization::deserialize<T>(json_format, ref(reader), util::forward<Args>(args)...);
        }
    };
}

template<typename T = json::Value>
constexpr inline auto deserialize_json = detail::DeserializeJsonFunction<T> {};
}

namespace di {
inline namespace literals {
    inline namespace json_literals {
        namespace detail {
            template<container::FixedString string>
            consteval bool valid_json_literal() {
                // NOTE: GCC does not think that the following is a constant expression, but clang does.
#ifdef DI_CLANG
                auto string_view = container::fixed_string_to_utf8_string_view<string>();
                return serialization::from_json_string<>(string_view).has_value();
#endif
                return true;
            }
        }

        template<container::FixedString string>
        requires(detail::valid_json_literal<string>())
        constexpr auto operator""_json() {
            auto string_view = container::fixed_string_to_utf8_string_view<string>();
            return *serialization::from_json_string<>(string_view);
        }
    }
}
}

#if !defined(DI_NO_GLOBALS) && !defined(DI_NO_GLOBAL_JSON_LITERALS)
using namespace di::literals::json_literals;
#endif

namespace di {
using serialization::JsonDeserializer;

using serialization::deserialize_json;
using serialization::from_json_string;
}
