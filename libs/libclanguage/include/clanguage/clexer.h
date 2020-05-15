#pragma once

#include <liim/string_view.h>
#include <liim/vector.h>

namespace CLanguage {

#define __ENUMERATE_C_KEYWORDS                     \
    __ENUMERATE_C_KEYWORD(Auto, "auto")            \
    __ENUMERATE_C_KEYWORD(Bool, "_Bool")           \
    __ENUMERATE_C_KEYWORD(Break, "case")           \
    __ENUMERATE_C_KEYWORD(Char, "char")            \
    __ENUMERATE_C_KEYWORD(Complex, "_Complex")     \
    __ENUMERATE_C_KEYWORD(Const, "const")          \
    __ENUMERATE_C_KEYWORD(Continue, "continue")    \
    __ENUMERATE_C_KEYWORD(Default, "default")      \
    __ENUMERATE_C_KEYWORD(Do, "do")                \
    __ENUMERATE_C_KEYWORD(Double, "double")        \
    __ENUMERATE_C_KEYWORD(Else, "else")            \
    __ENUMERATE_C_KEYWORD(Enum, "enum")            \
    __ENUMERATE_C_KEYWORD(Extern, "extern")        \
    __ENUMERATE_C_KEYWORD(Float, "float")          \
    __ENUMERATE_C_KEYWORD(For, "for")              \
    __ENUMERATE_C_KEYWORD(Goto, "goto")            \
    __ENUMERATE_C_KEYWORD(If, "if")                \
    __ENUMERATE_C_KEYWORD(Imaninary, "_Imaginary") \
    __ENUMERATE_C_KEYWORD(Inline, "inline")        \
    __ENUMERATE_C_KEYWORD(Int, "int")              \
    __ENUMERATE_C_KEYWORD(Long, "long")            \
    __ENUMERATE_C_KEYWORD(Register, "register")    \
    __ENUMERATE_C_KEYWORD(Restrict, "restrict")    \
    __ENUMERATE_C_KEYWORD(Return, "return")        \
    __ENUMERATE_C_KEYWORD(Short, "short")          \
    __ENUMERATE_C_KEYWORD(Signed, "signed")        \
    __ENUMERATE_C_KEYWORD(Sizeof, "sizeof")        \
    __ENUMERATE_C_KEYWORD(Static, "static")        \
    __ENUMERATE_C_KEYWORD(Struct, "struct")        \
    __ENUMERATE_C_KEYWORD(Switch, "switch")        \
    __ENUMERATE_C_KEYWORD(Typedef, "typedef")      \
    __ENUMERATE_C_KEYWORD(Union, "union")          \
    __ENUMERATE_C_KEYWORD(Unsigned, "unsigned")    \
    __ENUMERATE_C_KEYWORD(Void, "void")            \
    __ENUMERATE_C_KEYWORD(Volatile, "volatile")    \
    __ENUMERATE_C_KEYWORD(While, "while")

#define __ENUMERATE_C_OPS                     \
    __ENUMERATE_C_OP(RightShiftAssign, ">>=") \
    __ENUMERATE_C_OP(LeftShiftAssign, "<<=")  \
    __ENUMERATE_C_OP(AddAssign, "+=")         \
    __ENUMERATE_C_OP(SubAssign, "-=")         \
    __ENUMERATE_C_OP(MultAssign, "*=")        \
    __ENUMERATE_C_OP(DivAssign, "/=")         \
    __ENUMERATE_C_OP(ModAssign, "%=")         \
    __ENUMERATE_C_OP(AndAssign, "&=")         \
    __ENUMERATE_C_OP(XorAssign, "^=")         \
    __ENUMERATE_C_OP(OrAssign, "|=")          \
    __ENUMERATE_C_OP(RightShift, ">>")        \
    __ENUMERATE_C_OP(LeftShift, "<<")         \
    __ENUMERATE_C_OP(PlusPlus, "++")          \
    __ENUMERATE_C_OP(MinusMinus, "--")        \
    __ENUMERATE_C_OP(Arrow, "->")             \
    __ENUMERATE_C_OP(LogicalAnd, "&&")        \
    __ENUMERATE_C_OP(LogicalOr, "||")         \
    __ENUMERATE_C_OP(LessThanEqual, "<=")     \
    __ENUMERATE_C_OP(GreaterThanEqual, ">=")  \
    __ENUMERATE_C_OP(Equal, "==")             \
    __ENUMERATE_C_OP(NotEqual, "!=")          \
    __ENUMERATE_C_OP(Comma, ",")              \
    __ENUMERATE_C_OP(Assign, "=")             \
    __ENUMERATE_C_OP(Dot, ".")                \
    __ENUMERATE_C_OP(And, "&")                \
    __ENUMERATE_C_OP(Not, "!")                \
    __ENUMERATE_C_OP(Negate, "~")             \
    __ENUMERATE_C_OP(Minus, "-")              \
    __ENUMERATE_C_OP(Plus, "+")               \
    __ENUMERATE_C_OP(Mult, "*")               \
    __ENUMERATE_C_OP(Div, "/")                \
    __ENUMERATE_C_OP(Modulo, "%")             \
    __ENUMERATE_C_OP(LessThan, "<")           \
    __ENUMERATE_C_OP(GreaterThan, ">")        \
    __ENUMERATE_C_OP(Xor, "^")                \
    __ENUMERATE_C_OP(Or, "|")                 \
    __ENUMERATE_C_OP(Ternary, "?")            \
    __ENUMERATE_C_OP(Ellipsis, "...")         \
    __ENUMERATE_C_OP(Semicolon, ";")          \
    __ENUMERATE_C_OP(RightBrace, "{")         \
    __ENUMERATE_C_OP(LeftBrace, "}")          \
    __ENUMERATE_C_OP(Colon, ":")              \
    __ENUMERATE_C_OP(LeftParenthesis, "(")    \
    __ENUMERATE_C_OP(RightParenthesis, ")")   \
    __ENUMERATE_C_OP(LeftBracket, "[")        \
    __ENUMERATE_C_OP(RightBracket, "]")

struct CToken {
    enum class Type {
#undef __ENUMERATE_C_KEYWORD
#define __ENUMERATE_C_KEYWORD(n, s) Keyword##n,
        __ENUMERATE_C_KEYWORDS NumericLiteral,
        StringLiteral,
        CharacterLiteral,
        Identifier,
        Comment,
#undef __ENUMERATE_C_OP
#define __ENUMERATE_C_OP(n, s) Operator##n,
        __ENUMERATE_C_OPS
    };

    int start_line;
    int start_col;
    int end_line;
    int end_col;
    Type type;
};

enum CLexMode { IgnoreComments, IncludeComments };

class CLexer {
public:
    CLexer(const char* input, size_t length) : m_input(input), m_length(length) {}

    bool lex(CLexMode mode = IgnoreComments);

    const Vector<CToken>& tokens() const { return m_tokens; }

private:
    int characters_remaining() const { return static_cast<int>(m_length - m_index); }

    bool input_starts_with(const StringView& view) const {
        if (characters_remaining() < view.size()) {
            return false;
        }

        return memcmp(m_input + m_index, view.start(), view.size()) == 0;
    }

    int peek(int n = 0) const {
        if (m_index + n >= m_length) {
            return EOF;
        }
        return m_input[m_index + n];
    }

    void consume() {
        if (peek() == '\n') {
            m_current_col = 0;
            m_current_line++;
        } else {
            m_current_col++;
        }

        m_index++;
    }

    void consume(int n) {
        for (int i = 0; i < n; i++) {
            consume();
        }
    }

    void begin_token() {
        m_token_start_line = m_current_line;
        m_token_start_col = m_current_col;
        m_token_started = true;
    }

    void commit_token(CToken::Type type) {
        if (!m_token_started) {
            return;
        }

        m_token_started = false;
        m_tokens.add({ m_token_start_line, m_token_start_col, m_current_line, m_current_col, type });
    }

    const char* m_input { nullptr };
    size_t m_index { 0 };
    size_t m_length { 0 };

    int m_token_start_line { 0 };
    int m_token_start_col { 0 };
    int m_current_line { 0 };
    int m_current_col { 0 };
    bool m_token_started { false };
    Vector<CToken> m_tokens;
};

}
