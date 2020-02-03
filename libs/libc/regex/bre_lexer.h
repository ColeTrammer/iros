#pragma once

#include <assert.h>
#include <liim/vector.h>
#include <parser/generic_lexer.h>

#include "basic_token_type.h"
#include "bre_value.h"

class BRELexer final : public GenericLexer<BasicTokenType, BREValue> {
public:
    using Token = GenericToken<BasicTokenType, BREValue>;

    BRELexer(const char* regex) : m_input_stream(regex) {};
    virtual ~BRELexer();

    bool lex();

    virtual BasicTokenType peek_next_token_type() const override {
        if (m_current_position_to_parser >= (size_t) m_tokens.size()) {
            return BasicTokenType::End;
        }

        BasicTokenType type = m_tokens[m_current_position_to_parser].type();
        return type;
    }

    virtual const BREValue& peek_next_token_value() const override {
        assert(m_current_position_to_parser < (size_t) m_tokens.size());
        return m_tokens[m_current_position_to_parser].value();
    }

    virtual void advance() override { m_current_position_to_parser++; }

    int error_code() const { return m_error_code; }

private:
    const char* m_input_stream;
    Vector<Token> m_tokens;
    int m_error_code { 0 };
    size_t m_current_position_to_parser { 0 };
};