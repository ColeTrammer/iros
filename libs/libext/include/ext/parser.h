#pragma once

#include <ext/forward.h>
#include <liim/error.h>
#include <liim/error/typed_domain.h>
#include <liim/format.h>
#include <liim/result.h>
#include <liim/string.h>
#include <liim/string_view.h>
#include <liim/try.h>

namespace Ext {
class ParserErrorDomain : public TypedErrorDomain<ParserErrorDomain, UniquePtr<ParserError>> {
public:
    constexpr virtual ~ParserErrorDomain() override {}

    virtual ErasedString type() const override { return "ParserErorr"sv; }
    ErasedString message(const UniquePtr<ParserError>& value) const;
};

constexpr inline ParserErrorDomain parser_error_domain {};

class ParserError {
public:
    ParserError(StringView working_buffer, StringView original_buffer, String message)
        : m_working_buffer(working_buffer), m_original_buffer(original_buffer), m_message(message) {}

    StringView message() const { return m_message.view(); }

    String to_message() const { return format("Error parsing `{}': {}", m_original_buffer, m_message); }

private:
    friend Error<> tag_invoke(Tag<into_erased_error>, ParserError&& value) {
        return ParserErrorDomain::Error(make_unique<ParserError>(move(value)), &parser_error_domain);
    }

    String m_working_buffer;
    String m_original_buffer;
    String m_message;
};

inline ErasedString ParserErrorDomain::message(const UniquePtr<ParserError>& value) const {
    return value->to_message();
}

class Parser {
public:
    constexpr explicit Parser(StringView buffer) : m_working_buffer(buffer), m_original_buffer(buffer) {}

    ParserError error(String message) const { return ParserError(m_working_buffer, m_original_buffer, move(message)); }
    Err<ParserError> result_error(String message) const { return Err(error(move(message))); }

    Result<StringView, ParserError> consume(StringView view) {
        return try_consume(view).unwrap_or_else([&] {
            return error(format("Expected token `{}'", view));
        });
    }

    Result<void, ParserError> consume_end() const {
        if (peek()) {
            return result_error(format("Expected end of input"));
        }
        return {};
    }

    constexpr Option<StringView> peek(size_t n = 1) const {
        if (m_working_buffer.size() < n) {
            return None {};
        }
        return m_working_buffer.first(n);
    }

    constexpr Option<StringView> try_consume(StringView view) {
        if (peek(view.size()) != view) {
            return None {};
        }
        return try_consume(view.size());
    }

    constexpr Option<StringView> try_consume(size_t n = 1) {
        if (m_working_buffer.size() < n) {
            return None {};
        }
        auto result = m_working_buffer.first(n);
        m_working_buffer = m_working_buffer.substring(n);
        return result;
    }

    constexpr StringView consume_matching(StringView set) {
        size_t count = 0;
        for (;; count++) {
            auto last_char = peek(count + 1).map([](auto& sv) {
                return sv.last();
            });
            if (!last_char || !set.index_of(*last_char)) {
                break;
            }
        }
        return *try_consume(count);
    }

    constexpr StringView consume_all() { return *try_consume(m_working_buffer.size()); }

private:
    StringView m_working_buffer;
    StringView m_original_buffer;
};

template<typename T>
struct ParserAdapter {};

template<typename T>
Result<T, ParserError> parse_partial(Parser& parser) {
    return ParserAdapter<T>::parse(parser);
}

template<typename T>
Result<T, ParserError> parse(StringView input) {
    auto parser = Ext::Parser { input };
    auto result = TRY(parse_partial<T>(parser));
    TRY(parser.consume_end());
    return result;
}

template<>
struct ParserAdapter<int> {
    static Result<int, ParserError> parse(Parser& parser) {
        int sign = 1;
        if (parser.try_consume("-")) {
            sign = -1;
        } else {
            parser.try_consume("+");
        }

        auto digits = parser.consume_matching("0123456789");
        if (digits.empty()) {
            return parser.result_error("Expected a numerical digit");
        }

        // FIXME: handle overflow
        int result = 0;
        for (char c : digits) {
            result *= 10;
            result += (c - '0');
        }
        return sign * result;
    }
};

template<typename T>
requires(LIIM::IsOneOf<T, String, StringView>::value) struct ParserAdapter<T> {
    static Result<T, ParserError> parse(Parser& parser) { return T { parser.consume_all() }; }
};
}
