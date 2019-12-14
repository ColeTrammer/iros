#pragma once

#include <liim/string_view.h>

template<typename Type> struct Token {
    Token(Type _type, StringView _view) : type(_type), text(_view) {}
    Token(const Token& other) : type(other.type), text(other.text) {}

    Type type;
    StringView text;
};