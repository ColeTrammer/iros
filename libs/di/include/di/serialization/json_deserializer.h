#pragma once

#include <di/any/concepts/impl.h>
#include <di/concepts/constructible_from.h>
#include <di/container/string/encoding.h>
#include <di/container/string/fixed_string.h>
#include <di/container/string/fixed_string_to_utf8_string_view.h>
#include <di/container/string/string_view.h>
#include <di/io/interface/reader.h>
#include <di/io/string_reader.h>
#include <di/serialization/json_value.h>
#include <di/types/prelude.h>
#include <di/util/exchange.h>
#include <di/util/to_underlying.h>
#include <di/vocab/error/generic_domain.h>
#include <di/vocab/optional/nullopt.h>
#include <di/vocab/optional/optional_forward_declaration.h>

namespace di::serialization {
/// A deserializer for the JSON format.
///
/// @tparam Reader The type of the reader to read from.
///
/// This implements the JSON grammar as specified in [https://www.rfc-editor.org/rfc/rfc8259](RFC 8259).
template<concepts::Impl<io::Reader> Reader>
class JsonDeserializer {
private:
    template<typename T>
    using Result = meta::ReadResult<T, Reader>;

public:
    template<typename T>
    requires(concepts::ConstructibleFrom<Reader, T>)
    constexpr explicit JsonDeserializer(T&& reader) : m_reader(util::forward<T>(reader)) {}

    constexpr Result<json::Value> deserialize() {
        auto result = DI_TRY(deserialize_value());
        DI_TRY(skip_whitespace());
        if (!at_end) {
            return vocab::Unexpected(vocab::BasicError::Invalid);
        }
        return result;
    }

private:
    constexpr static bool is_whitespace(c32 code_point) {
        return code_point == ' ' || code_point == '\t' || code_point == '\n' || code_point == '\r';
    }

    constexpr Result<void> expect(c32 expected) {
        auto code_point = DI_TRY(next_code_point());
        if (!code_point || *code_point != expected) {
            return vocab::Unexpected(vocab::BasicError::Invalid);
        }
        return {};
    }

    constexpr Result<void> fill_next_code_point() {
        // FIXME: handle UTF-8.
        auto byte = vocab::Array<types::byte, 1> {};
        auto nread = DI_TRY(io::read_some(m_reader, byte));
        if (nread == 0) {
            at_end = true;
        }
        m_next_code_point = c32(byte[0]);
        return {};
    }

    constexpr Result<vocab::Optional<c32>> peek_next_code_point() {
        if (at_end) {
            return vocab::nullopt;
        }
        if (!m_next_code_point) {
            DI_TRY(fill_next_code_point());
            if (at_end) {
                return vocab::nullopt;
            }
        }
        return *m_next_code_point;
    }

    constexpr void consume() { m_next_code_point = vocab::nullopt; }

    constexpr Result<vocab::Optional<c32>> next_code_point() {
        if (at_end) {
            return vocab::nullopt;
        }
        if (!m_next_code_point) {
            DI_TRY(fill_next_code_point());
            if (at_end) {
                return vocab::nullopt;
            }
        }
        return *util::exchange(m_next_code_point, vocab::nullopt);
    }

    constexpr Result<c32> require_next_code_point() {
        auto code_point = DI_TRY(next_code_point());
        if (!code_point) {
            return vocab::Unexpected(vocab::BasicError::Invalid);
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
            return vocab::Unexpected(vocab::BasicError::Invalid);
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
                return deserialize_number();
            case U'{':
                return deserialize_object();
            case U'[':
                return deserialize_array();
            default:
                return vocab::Unexpected(vocab::BasicError::Invalid);
        }
    }

    constexpr Result<json::Null> deserialize_null() {
        DI_TRY(expect(U'n'));
        DI_TRY(expect(U'u'));
        DI_TRY(expect(U'l'));
        DI_TRY(expect(U'l'));
        return json::null;
    }

    constexpr Result<json::Bool> deserialize_true() {
        DI_TRY(expect(U't'));
        DI_TRY(expect(U'r'));
        DI_TRY(expect(U'u'));
        DI_TRY(expect(U'e'));
        return true;
    }

    constexpr Result<json::Bool> deserialize_false() {
        DI_TRY(expect(U'f'));
        DI_TRY(expect(U'a'));
        DI_TRY(expect(U'l'));
        DI_TRY(expect(U's'));
        DI_TRY(expect(U'e'));
        return false;
    }

    constexpr Result<json::String> deserialize_string() {
        DI_TRY(expect(U'"'));

        auto string = json::String {};
        for (;;) {
            auto code_point = DI_TRY(next_code_point());
            if (!code_point || *code_point < 0x20) {
                return vocab::Unexpected(vocab::BasicError::Invalid);
            }
            if (*code_point == U'"') {
                break;
            }
            // FIXME: handle escape sequences.
            string.push_back(*code_point);
        }
        return string;
    }

    constexpr Result<json::Number> deserialize_number() {
        auto first_code_point = DI_TRY(require_next_code_point());

        auto string = json::String {};
        if (first_code_point == U'-') {
            string.push_back(first_code_point);
            first_code_point = DI_TRY(require_next_code_point());
            if (first_code_point < U'0' || first_code_point > U'9') {
                return vocab::Unexpected(vocab::BasicError::Invalid);
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

        // FIXME: handle decimal point and exponent.
        auto result = parser::parse<json::Number>(string);
        if (!result) {
            return vocab::Unexpected(vocab::BasicError::Invalid);
        }
        return *result;
    }

    constexpr Result<json::Array> deserialize_array() {
        DI_TRY(expect(U'['));

        auto array = json::Array {};
        for (;;) {
            DI_TRY(skip_whitespace());
            auto code_point = DI_TRY(peek_next_code_point());
            if (!code_point) {
                return vocab::Unexpected(vocab::BasicError::Invalid);
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
        DI_TRY(expect(U'{'));

        auto object = json::Object {};
        for (;;) {
            DI_TRY(skip_whitespace());
            auto code_point = DI_TRY(peek_next_code_point());
            if (!code_point) {
                return vocab::Unexpected(vocab::BasicError::Invalid);
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
    bool at_end { false };
};

template<typename T>
JsonDeserializer(T&&) -> JsonDeserializer<T>;
}

namespace di {
inline namespace literals {
    inline namespace json_literals {
        namespace detail {
            template<container::FixedString string>
            consteval bool valid_json_literal() {
                // NOTE: GCC does not think that the following is a constant expression, but clang does.
#ifdef __clang__
                auto string_view = container::fixed_string_to_utf8_string_view<string>();
                auto reader = io::StringReader(string_view);
                auto deserializer = serialization::JsonDeserializer(reader);
                return deserializer.deserialize().has_value();
#endif
                return true;
            }
        }

        template<container::FixedString string>
        requires(detail::valid_json_literal<string>())
        constexpr auto operator""_json() {
            auto string_view = container::fixed_string_to_utf8_string_view<string>();
            auto reader = io::StringReader(string_view);
            auto deserializer = serialization::JsonDeserializer(reader);
            return *deserializer.deserialize();
        }
    }
}
}

#if !defined(DI_NO_GLOBALS) && !defined(DI_NO_GLOBAL_JSON_LITERALS)
using namespace di::literals::json_literals;
#endif

namespace di {
using serialization::JsonDeserializer;
}
