#pragma once

#include <liim/string_view.h>

#define ENUMERATE_TOKEN_TYPES               \
    __ENUMERATE_TOKEN_TYPES(TokenMarker)    \
    __ENUMERATE_TOKEN_TYPES(StartMarker)    \
    __ENUMERATE_TOKEN_TYPES(PercentPercent) \
    __ENUMERATE_TOKEN_TYPES(Word)           \
    __ENUMERATE_TOKEN_TYPES(Colon)          \
    __ENUMERATE_TOKEN_TYPES(Pipe)           \
    __ENUMERATE_TOKEN_TYPES(Semicolon)      \
    __ENUMERATE_TOKEN_TYPES(Literal)

struct Token {
    enum class Type {
#undef __ENUMERATE_TOKEN_TYPES
#define __ENUMERATE_TOKEN_TYPES(t) Token##t,
        ENUMERATE_TOKEN_TYPES Invalid
    };

    Token(Type _type, StringView _view) : type(_type), text(_view) {}
    Token(const Token& other) : type(other.type), text(other.text) {}

    Type type;
    StringView text;
};