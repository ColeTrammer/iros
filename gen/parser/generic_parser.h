#pragma once

#include <liim/either.h>
#include <liim/vector.h>
#include <stddef.h>

#include "token.h"

template<typename TokenType> class GenericParser {
public:
    using ParseResult = Either<StringView, size_t>;

    explicit GenericParser(Vector<Token<TokenType>> tokens) : m_tokens(tokens) {}
    virtual ~GenericParser() {}

    virtual ParseResult parse() = 0;

protected:
    const Token<TokenType>& peek() const { return m_tokens[m_pos]; }
    void consume_token() { m_pos++; }

    ParseResult try_parse_literal(const StringView& literal) {
        if (peek().text == literal) {
            consume_token();
            return { literal };
        }

        return { m_pos };
    }

private:
    Vector<Token<TokenType>> m_tokens;
    size_t m_pos { 0 };
};