#pragma once

#define ENUMERATE_TOKEN_TYPES               \
    __ENUMERATE_TOKEN_TYPES(TokenMarker)    \
    __ENUMERATE_TOKEN_TYPES(StartMarker)    \
    __ENUMERATE_TOKEN_TYPES(PercentPercent) \
    __ENUMERATE_TOKEN_TYPES(Word)           \
    __ENUMERATE_TOKEN_TYPES(Colon)          \
    __ENUMERATE_TOKEN_TYPES(Pipe)           \
    __ENUMERATE_TOKEN_TYPES(Semicolon)      \
    __ENUMERATE_TOKEN_TYPES(Literal)

enum class TokenType {
#define _(x) x
#undef __ENUMERATE_TOKEN_TYPES
#define __ENUMERATE_TOKEN_TYPES(t) _(Token##t),
    ENUMERATE_TOKEN_TYPES Invalid
};