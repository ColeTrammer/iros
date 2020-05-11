#pragma once

#include <assert.h>
#include <liim/string_view.h>

#define ENUMERATE_LITERALS                        \
    __ENUMERATE_LITERALS('!', ExclamationMark)    \
    __ENUMERATE_LITERALS('"', DoubleQuote)        \
    __ENUMERATE_LITERALS('#', Pound)              \
    __ENUMERATE_LITERALS('$', Dollar)             \
    __ENUMERATE_LITERALS('%', Percent)            \
    __ENUMERATE_LITERALS('&', Ampersand)          \
    __ENUMERATE_LITERALS('\'', SingleQuote)       \
    __ENUMERATE_LITERALS('(', LeftParenthesis)    \
    __ENUMERATE_LITERALS(')', RightParenthesis)   \
    __ENUMERATE_LITERALS('*', Asterisk)           \
    __ENUMERATE_LITERALS('+', Plus)               \
    __ENUMERATE_LITERALS(',', Comma)              \
    __ENUMERATE_LITERALS('-', Minus)              \
    __ENUMERATE_LITERALS('.', Period)             \
    __ENUMERATE_LITERALS('/', ForwardSlash)       \
    __ENUMERATE_LITERALS('0', Zero)               \
    __ENUMERATE_LITERALS('1', One)                \
    __ENUMERATE_LITERALS('2', Two)                \
    __ENUMERATE_LITERALS('3', Three)              \
    __ENUMERATE_LITERALS('4', Four)               \
    __ENUMERATE_LITERALS('5', Five)               \
    __ENUMERATE_LITERALS('6', Six)                \
    __ENUMERATE_LITERALS('7', Seven)              \
    __ENUMERATE_LITERALS('8', Eight)              \
    __ENUMERATE_LITERALS('9', Nine)               \
    __ENUMERATE_LITERALS(':', Colon)              \
    __ENUMERATE_LITERALS(';', Semicolon)          \
    __ENUMERATE_LITERALS('<', LessThan)           \
    __ENUMERATE_LITERALS('=', Equal)              \
    __ENUMERATE_LITERALS('>', GreaterThan)        \
    __ENUMERATE_LITERALS('?', QuestionMark)       \
    __ENUMERATE_LITERALS('@', AtSign)             \
    __ENUMERATE_LITERALS('A', UpperCaseA)         \
    __ENUMERATE_LITERALS('B', UpperCaseB)         \
    __ENUMERATE_LITERALS('C', UpperCaseC)         \
    __ENUMERATE_LITERALS('D', UpperCaseD)         \
    __ENUMERATE_LITERALS('E', UpperCaseE)         \
    __ENUMERATE_LITERALS('F', UpperCaseF)         \
    __ENUMERATE_LITERALS('G', UpperCaseG)         \
    __ENUMERATE_LITERALS('H', UpperCaseH)         \
    __ENUMERATE_LITERALS('I', UpperCaseI)         \
    __ENUMERATE_LITERALS('J', UpperCaseJ)         \
    __ENUMERATE_LITERALS('K', UpperCaseK)         \
    __ENUMERATE_LITERALS('L', UpperCaseL)         \
    __ENUMERATE_LITERALS('M', UpperCaseM)         \
    __ENUMERATE_LITERALS('N', UpperCaseN)         \
    __ENUMERATE_LITERALS('O', UpperCaseO)         \
    __ENUMERATE_LITERALS('P', UpperCaseP)         \
    __ENUMERATE_LITERALS('Q', UpperCaseQ)         \
    __ENUMERATE_LITERALS('R', UpperCaseR)         \
    __ENUMERATE_LITERALS('S', UpperCaseS)         \
    __ENUMERATE_LITERALS('T', UpperCaseT)         \
    __ENUMERATE_LITERALS('U', UpperCaseU)         \
    __ENUMERATE_LITERALS('V', UpperCaseV)         \
    __ENUMERATE_LITERALS('W', UpperCaseW)         \
    __ENUMERATE_LITERALS('X', UpperCaseX)         \
    __ENUMERATE_LITERALS('Y', UpperCaseY)         \
    __ENUMERATE_LITERALS('Z', UpperCaseZ)         \
    __ENUMERATE_LITERALS('[', LeftSquareBracket)  \
    __ENUMERATE_LITERALS('\\', BackSlash)         \
    __ENUMERATE_LITERALS(']', RightSquareBracket) \
    __ENUMERATE_LITERALS('^', Carrot)             \
    __ENUMERATE_LITERALS('_', Underscore)         \
    __ENUMERATE_LITERALS('`', BackQuote)          \
    __ENUMERATE_LITERALS('a', LowerCaseA)         \
    __ENUMERATE_LITERALS('b', LowerCaseB)         \
    __ENUMERATE_LITERALS('c', LowerCaseC)         \
    __ENUMERATE_LITERALS('d', LowerCaseD)         \
    __ENUMERATE_LITERALS('e', LowerCaseE)         \
    __ENUMERATE_LITERALS('f', LowerCaseF)         \
    __ENUMERATE_LITERALS('g', LowerCaseG)         \
    __ENUMERATE_LITERALS('h', LowerCaseH)         \
    __ENUMERATE_LITERALS('i', LowerCaseI)         \
    __ENUMERATE_LITERALS('j', LowerCaseJ)         \
    __ENUMERATE_LITERALS('k', LowerCaseK)         \
    __ENUMERATE_LITERALS('l', LowerCaseL)         \
    __ENUMERATE_LITERALS('m', LowerCaseM)         \
    __ENUMERATE_LITERALS('n', LowerCaseN)         \
    __ENUMERATE_LITERALS('o', LowerCaseO)         \
    __ENUMERATE_LITERALS('p', LowerCaseP)         \
    __ENUMERATE_LITERALS('q', LowerCaseQ)         \
    __ENUMERATE_LITERALS('r', LowerCaseR)         \
    __ENUMERATE_LITERALS('s', LowerCaseS)         \
    __ENUMERATE_LITERALS('t', LowerCaseT)         \
    __ENUMERATE_LITERALS('u', LowerCaseU)         \
    __ENUMERATE_LITERALS('v', LowerCaseV)         \
    __ENUMERATE_LITERALS('w', LowerCaseW)         \
    __ENUMERATE_LITERALS('x', LowerCaseX)         \
    __ENUMERATE_LITERALS('y', LowerCaseY)         \
    __ENUMERATE_LITERALS('z', LowerCaseZ)         \
    __ENUMERATE_LITERALS('{', LeftCurlyBrace)     \
    __ENUMERATE_LITERALS('|', Pipe)               \
    __ENUMERATE_LITERALS('}', RightCurlyBrace)    \
    __ENUMERATE_LITERALS('~', Tilde)

static inline String literal_to_token(const StringView& literal) {
    String translated = "";
    for (int i = 0; i < literal.size(); i++) {
#undef __ENUMERATE_LITERALS
#define __ENUMERATE_LITERALS(c, n) \
    if (literal.start()[i] == c)   \
        translated += #n;
        ENUMERATE_LITERALS
    }
    return translated;
}

static inline String token_to_literal(const StringView& name) {
    String output = "";
    for (int i = 0; i < name.size();) {
        StringView next = StringView(&name.start()[i], name.end());
#undef __ENUMERATE_LITERALS
#define __ENUMERATE_LITERALS(c, n)          \
    if (next.starts_with(StringView(#n))) { \
        if (strcmp(#n, "BackSlash") == 0) { \
            output += String("\\\\");       \
        } else {                            \
            output += String(c);            \
        }                                   \
        i += StringView(#n).size();         \
        continue;                           \
    }
        ENUMERATE_LITERALS
        break;
    }
    return output;
}
