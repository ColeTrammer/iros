#pragma once

#include <assert.h>
#include <liim/hash_map.h>
#include <liim/string.h>
#include <liim/string_view.h>
#include <liim/vector.h>
#include <parser/generic_lexer.h>

#include "regex_token_type.h"
#include "regex_value.h"

class RegexLexer final : public GenericLexer<RegexTokenType, RegexValue> {
public:
    using Token = GenericToken<RegexTokenType, RegexValue>;

    RegexLexer(const char* regex, int cflags) : m_input_stream(regex), m_flags(cflags) {};
    virtual ~RegexLexer();

    bool lex();

    virtual RegexTokenType peek_next_token_type() const override {
        if (m_current_position_to_parser >= (size_t) m_tokens.size()) {
            return RegexTokenType::End;
        }

        RegexTokenType type = m_tokens[m_current_position_to_parser].type();
        return type;
    }

    virtual const RegexValue& peek_next_token_value() const override {
        assert(m_current_position_to_parser < (size_t) m_tokens.size());
        return m_tokens[m_current_position_to_parser].value();
    }

    virtual void advance() override { m_current_position_to_parser++; }

    int error_code() const { return m_error_code; }
    int num_sub_expressions() const { return m_group_count; }

    int group_at_position(size_t index) const {
        ssize_t i = static_cast<ssize_t>(index);
        for (; i >= 0; i--) {
            const int* group_index = m_group_incidices.get(static_cast<size_t>(i));
            if (group_index) {
                return *group_index;
            }
        }
        return 0;
    }

    const Vector<Token>& tokens() const { return m_tokens; }

private:
    StringView peek(int n) const { return { &m_input_stream[m_position], &m_input_stream[m_position + n] }; }
    char peek() const { return m_input_stream[m_position]; }
    char prev() const { return m_input_stream[m_position - 1]; }
    void consume() { m_position++; }
    void commit_token(RegexTokenType type) {
        if (type == RegexTokenType::QuotedCharacter || type == RegexTokenType::BackReference) {
            m_token_start++;
        }
        auto text = StringView { m_input_stream.string() + m_token_start, m_input_stream.string() + m_position };
        m_tokens.add(Token { type, RegexValue { TokenInfo { text, m_token_start } } });
        m_token_start = m_position;
    }

    String m_input_stream;
    size_t m_position { 0 };
    size_t m_token_start { 0 };
    Vector<Token> m_tokens;
    int m_error_code { 0 };
    int m_group_count { 0 };
    int m_flags { 0 };
    size_t m_current_position_to_parser { 0 };
    HashMap<size_t, int> m_group_incidices;
};
