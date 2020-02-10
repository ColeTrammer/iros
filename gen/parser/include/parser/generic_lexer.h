#pragma once

#include <parser/generic_token.h>

template<typename TokenType, typename Value>
class GenericLexer {
public:
    GenericLexer() {}
    virtual ~GenericLexer() {}

    virtual TokenType peek_next_token_type() const = 0;
    virtual const Value& peek_next_token_value() const = 0;
    virtual void advance() = 0;
};