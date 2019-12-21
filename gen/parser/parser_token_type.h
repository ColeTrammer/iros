#pragma once

#define ENUMERATE_TOKEN_TYPES                 \
    __ENUMERATE_TOKEN_TYPES(TokenMarker)      \
    __ENUMERATE_TOKEN_TYPES(StartMarker)      \
    __ENUMERATE_TOKEN_TYPES(PercentPercent)   \
    __ENUMERATE_TOKEN_TYPES(Word)             \
    __ENUMERATE_TOKEN_TYPES(Lhs)              \
    __ENUMERATE_TOKEN_TYPES(Colon)            \
    __ENUMERATE_TOKEN_TYPES(Pipe)             \
    __ENUMERATE_TOKEN_TYPES(LeftParenthesis)  \
    __ENUMERATE_TOKEN_TYPES(RightParenthesis) \
    __ENUMERATE_TOKEN_TYPES(Plus)             \
    __ENUMERATE_TOKEN_TYPES(Star)             \
    __ENUMERATE_TOKEN_TYPES(QuestionMark)     \
    __ENUMERATE_TOKEN_TYPES(Semicolon)        \
    __ENUMERATE_TOKEN_TYPES(Literal)          \
    __ENUMERATE_TOKEN_TYPES(End)

enum class TokenType {
#define _(x) x
#undef __ENUMERATE_TOKEN_TYPES
#define __ENUMERATE_TOKEN_TYPES(t) _(Token##t),
    ENUMERATE_TOKEN_TYPES Invalid
};