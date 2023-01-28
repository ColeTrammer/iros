#pragma once

#include <liim/string_view.h>
#include <liim/vector.h>

namespace CLanguage {

#define __ENUMERATE_CPP_KEYWORDS                                 \
    __ENUMERATE_CPP_KEYWORD(Alignas, "alignas")                  \
    __ENUMERATE_CPP_KEYWORD(Alignof, "alignof")                  \
    __ENUMERATE_CPP_KEYWORD(And, "and")                          \
    __ENUMERATE_CPP_KEYWORD(AndEq, "and_eq")                     \
    __ENUMERATE_CPP_KEYWORD(Asm, "asm")                          \
    __ENUMERATE_CPP_KEYWORD(Auto, "auto")                        \
    __ENUMERATE_CPP_KEYWORD(BitAnd, "bitand")                    \
    __ENUMERATE_CPP_KEYWORD(BitOr, "bitor")                      \
    __ENUMERATE_CPP_KEYWORD(Break, "break")                      \
    __ENUMERATE_CPP_KEYWORD(Case, "case")                        \
    __ENUMERATE_CPP_KEYWORD(Catch, "catch")                      \
    __ENUMERATE_CPP_KEYWORD(Class, "class")                      \
    __ENUMERATE_CPP_KEYWORD(Compl, "compl")                      \
    __ENUMERATE_CPP_KEYWORD(Concept, "concept")                  \
    __ENUMERATE_CPP_KEYWORD(ConstEval, "consteval")              \
    __ENUMERATE_CPP_KEYWORD(ConstExpr, "constexpr")              \
    __ENUMERATE_CPP_KEYWORD(ConstInit, "constinit")              \
    __ENUMERATE_CPP_KEYWORD(ConstCast, "const_cast")             \
    __ENUMERATE_CPP_KEYWORD(Continue, "continue")                \
    __ENUMERATE_CPP_KEYWORD(CoAwait, "co_await")                 \
    __ENUMERATE_CPP_KEYWORD(CoReturn, "co_return")               \
    __ENUMERATE_CPP_KEYWORD(CoYield, "co_yield")                 \
    __ENUMERATE_CPP_KEYWORD(Decltype, "decltype")                \
    __ENUMERATE_CPP_KEYWORD(Default, "default")                  \
    __ENUMERATE_CPP_KEYWORD(Delete, "delete")                    \
    __ENUMERATE_CPP_KEYWORD(Do, "do")                            \
    __ENUMERATE_CPP_KEYWORD(DynamicCast, "dynamic_cast")         \
    __ENUMERATE_CPP_KEYWORD(Else, "else")                        \
    __ENUMERATE_CPP_KEYWORD(Enum, "enum")                        \
    __ENUMERATE_CPP_KEYWORD(Explicit, "explicit")                \
    __ENUMERATE_CPP_KEYWORD(Export, "export")                    \
    __ENUMERATE_CPP_KEYWORD(Extern, "extern")                    \
    __ENUMERATE_CPP_KEYWORD(Final, "final")                      \
    __ENUMERATE_CPP_KEYWORD(For, "for")                          \
    __ENUMERATE_CPP_KEYWORD(Friend, "friend")                    \
    __ENUMERATE_CPP_KEYWORD(Goto, "goto")                        \
    __ENUMERATE_CPP_KEYWORD(If, "if")                            \
    __ENUMERATE_CPP_KEYWORD(Inline, "inline")                    \
    __ENUMERATE_CPP_KEYWORD(Mutable, "mutable")                  \
    __ENUMERATE_CPP_KEYWORD(Namespace, "namespace")              \
    __ENUMERATE_CPP_KEYWORD(New, "new")                          \
    __ENUMERATE_CPP_KEYWORD(NoExcept, "noexcept")                \
    __ENUMERATE_CPP_KEYWORD(Not, "not")                          \
    __ENUMERATE_CPP_KEYWORD(NotEq, "not_eq")                     \
    __ENUMERATE_CPP_KEYWORD(Operator, "operator")                \
    __ENUMERATE_CPP_KEYWORD(Or, "or")                            \
    __ENUMERATE_CPP_KEYWORD(OrEq, "or_eq")                       \
    __ENUMERATE_CPP_KEYWORD(Override, "override")                \
    __ENUMERATE_CPP_KEYWORD(Private, "private")                  \
    __ENUMERATE_CPP_KEYWORD(Public, "public")                    \
    __ENUMERATE_CPP_KEYWORD(Proected, "protected")               \
    __ENUMERATE_CPP_KEYWORD(ReinterpretCast, "reinterpret_cast") \
    __ENUMERATE_CPP_KEYWORD(Requires, "requires")                \
    __ENUMERATE_CPP_KEYWORD(Return, "return")                    \
    __ENUMERATE_CPP_KEYWORD(Sizeof, "sizeof")                    \
    __ENUMERATE_CPP_KEYWORD(Static, "static")                    \
    __ENUMERATE_CPP_KEYWORD(StaticAssert, "static_assert")       \
    __ENUMERATE_CPP_KEYWORD(StaticCast, "static_cast")           \
    __ENUMERATE_CPP_KEYWORD(Struct, "struct")                    \
    __ENUMERATE_CPP_KEYWORD(Switch, "switch")                    \
    __ENUMERATE_CPP_KEYWORD(Template, "template")                \
    __ENUMERATE_CPP_KEYWORD(ThreadLocal, "thread_local")         \
    __ENUMERATE_CPP_KEYWORD(Throw, "throw")                      \
    __ENUMERATE_CPP_KEYWORD(Try, "try")                          \
    __ENUMERATE_CPP_KEYWORD(TypeDef, "typedef")                  \
    __ENUMERATE_CPP_KEYWORD(TypeId, "typeid")                    \
    __ENUMERATE_CPP_KEYWORD(TypeName, "typename")                \
    __ENUMERATE_CPP_KEYWORD(Using, "using")                      \
    __ENUMERATE_CPP_KEYWORD(Virtual, "virtual")                  \
    __ENUMERATE_CPP_KEYWORD(While, "while")                      \
    __ENUMERATE_CPP_KEYWORD(Xor, "xor")                          \
    __ENUMERATE_CPP_KEYWORD(XorEq, "xor_eq")

#define __ENUMERATE_CPP_TYPE_TOKENS                  \
    __ENUMERATE_CPP_TYPE_TOKEN(Bool, "bool")         \
    __ENUMERATE_CPP_TYPE_TOKEN(Char, "char")         \
    __ENUMERATE_CPP_TYPE_TOKEN(Char8, "char8_t")     \
    __ENUMERATE_CPP_TYPE_TOKEN(Char16, "char16_t")   \
    __ENUMERATE_CPP_TYPE_TOKEN(Char32, "char32_t")   \
    __ENUMERATE_CPP_TYPE_TOKEN(Const, "const")       \
    __ENUMERATE_CPP_TYPE_TOKEN(Double, "double")     \
    __ENUMERATE_CPP_TYPE_TOKEN(Float, "float")       \
    __ENUMERATE_CPP_TYPE_TOKEN(Int, "int")           \
    __ENUMERATE_CPP_TYPE_TOKEN(Long, "long")         \
    __ENUMERATE_CPP_TYPE_TOKEN(Register, "register") \
    __ENUMERATE_CPP_TYPE_TOKEN(Short, "short")       \
    __ENUMERATE_CPP_TYPE_TOKEN(Signed, "signed")     \
    __ENUMERATE_CPP_TYPE_TOKEN(Union, "union")       \
    __ENUMERATE_CPP_TYPE_TOKEN(Unsigned, "unsigned") \
    __ENUMERATE_CPP_TYPE_TOKEN(Void, "void")         \
    __ENUMERATE_CPP_TYPE_TOKEN(WChar, "wchar_t")

#define __ENUMERATE_CPP_LITERALS                \
    __ENUMERATE_CPP_LITERAL(False, "false")     \
    __ENUMERATE_CPP_LITERAL(NullPtr, "nullptr") \
    __ENUMERATE_CPP_LITERAL(This, "this")       \
    __ENUMERATE_CPP_LITERAL(True, "true")

#define __ENUMERATE_CPP_OPS                     \
    __ENUMERATE_CPP_OP(Scope, "::")             \
    __ENUMERATE_CPP_OP(Spaceship, "<=>")        \
    __ENUMERATE_CPP_OP(RightShiftAssign, ">>=") \
    __ENUMERATE_CPP_OP(LeftShiftAssign, "<<=")  \
    __ENUMERATE_CPP_OP(AddAssign, "+=")         \
    __ENUMERATE_CPP_OP(SubAssign, "-=")         \
    __ENUMERATE_CPP_OP(MultAssign, "*=")        \
    __ENUMERATE_CPP_OP(DivAssign, "/=")         \
    __ENUMERATE_CPP_OP(ModAssign, "%=")         \
    __ENUMERATE_CPP_OP(AndAssign, "&=")         \
    __ENUMERATE_CPP_OP(XorAssign, "^=")         \
    __ENUMERATE_CPP_OP(OrAssign, "|=")          \
    __ENUMERATE_CPP_OP(RightShift, ">>")        \
    __ENUMERATE_CPP_OP(LeftShift, "<<")         \
    __ENUMERATE_CPP_OP(PlusPlus, "++")          \
    __ENUMERATE_CPP_OP(MinusMinus, "--")        \
    __ENUMERATE_CPP_OP(Arrow, "->")             \
    __ENUMERATE_CPP_OP(LogicalAnd, "&&")        \
    __ENUMERATE_CPP_OP(LogicalOr, "||")         \
    __ENUMERATE_CPP_OP(LessThanEqual, "<=")     \
    __ENUMERATE_CPP_OP(GreaterThanEqual, ">=")  \
    __ENUMERATE_CPP_OP(Equal, "==")             \
    __ENUMERATE_CPP_OP(NotEqual, "!=")          \
    __ENUMERATE_CPP_OP(Comma, ",")              \
    __ENUMERATE_CPP_OP(Assign, "=")             \
    __ENUMERATE_CPP_OP(Dot, ".")                \
    __ENUMERATE_CPP_OP(And, "&")                \
    __ENUMERATE_CPP_OP(Not, "!")                \
    __ENUMERATE_CPP_OP(Negate, "~")             \
    __ENUMERATE_CPP_OP(Minus, "-")              \
    __ENUMERATE_CPP_OP(Plus, "+")               \
    __ENUMERATE_CPP_OP(Mult, "*")               \
    __ENUMERATE_CPP_OP(Div, "/")                \
    __ENUMERATE_CPP_OP(Modulo, "%")             \
    __ENUMERATE_CPP_OP(LessThan, "<")           \
    __ENUMERATE_CPP_OP(GreaterThan, ">")        \
    __ENUMERATE_CPP_OP(Xor, "^")                \
    __ENUMERATE_CPP_OP(Or, "|")                 \
    __ENUMERATE_CPP_OP(Ternary, "?")            \
    __ENUMERATE_CPP_OP(Ellipsis, "...")         \
    __ENUMERATE_CPP_OP(Semicolon, ";")          \
    __ENUMERATE_CPP_OP(RightBrace, "{")         \
    __ENUMERATE_CPP_OP(LeftBrace, "}")          \
    __ENUMERATE_CPP_OP(Colon, ":")              \
    __ENUMERATE_CPP_OP(LeftParenthesis, "(")    \
    __ENUMERATE_CPP_OP(RightParenthesis, ")")   \
    __ENUMERATE_CPP_OP(LeftBracket, "[")        \
    __ENUMERATE_CPP_OP(RightBracket, "]")

#define __ENUMERATE_CPP_PREPROCESSOR_KEYWORDS                \
    __ENUMERATE_CPP_PREPROCESSOR_KEYWORD(If, "if")           \
    __ENUMERATE_CPP_PREPROCESSOR_KEYWORD(IfDef, "ifdef")     \
    __ENUMERATE_CPP_PREPROCESSOR_KEYWORD(IfNDef, "ifndef")   \
    __ENUMERATE_CPP_PREPROCESSOR_KEYWORD(Elif, "elif")       \
    __ENUMERATE_CPP_PREPROCESSOR_KEYWORD(Else, "else")       \
    __ENUMERATE_CPP_PREPROCESSOR_KEYWORD(EndIf, "endif")     \
    __ENUMERATE_CPP_PREPROCESSOR_KEYWORD(Define, "define")   \
    __ENUMERATE_CPP_PREPROCESSOR_KEYWORD(Undef, "undef")     \
    __ENUMERATE_CPP_PREPROCESSOR_KEYWORD(Include, "include") \
    __ENUMERATE_CPP_PREPROCESSOR_KEYWORD(Pragma, "pragma")   \
    __ENUMERATE_CPP_PREPROCESSOR_KEYWORD(Error, "error")     \
    __ENUMERATE_CPP_PREPROCESSOR_KEYWORD(Line, "line")

struct CPPToken {
    enum class Type {
#undef __ENUMERATE_CPP_KEYWORD
#define __ENUMERATE_CPP_KEYWORD(n, s) Keyword##n,
        __ENUMERATE_CPP_KEYWORDS

#undef __ENUMERATE_CPP_TYPE_TOKEN
#define __ENUMERATE_CPP_TYPE_TOKEN(n, s) Type##n,
            __ENUMERATE_CPP_TYPE_TOKENS

#undef __ENUMERATE_CPP_LITERAL
#define __ENUMERATE_CPP_LITERAL(n, s) Literal##n,
                __ENUMERATE_CPP_LITERALS

#undef __ENUMERATE_CPP_OP
#define __ENUMERATE_CPP_OP(n, s) Operator##n,
                    __ENUMERATE_CPP_OPS

                        NumericLiteral,
        StringLiteral,
        CharacterLiteral,
        Identifier,
        Comment,

#undef __ENUMERATE_CPP_PREPROCESSOR_KEYWORD
#define __ENUMERATE_CPP_PREPROCESSOR_KEYWORD(n, s) Preprocessor##n,
        __ENUMERATE_CPP_PREPROCESSOR_KEYWORDS

            PreprocessorStart,
        PreprocessorPound,
        PreprocessorPoundPound,
        PreprocessorDefined,
        PreprocessorBackslash,
        PreprocessorSystemIncludeString,
    };

    int start_line;
    int start_col;
    int end_line;
    int end_col;
    Type type;
};

enum class CPPLexMode { IgnoreComments, IncludeComments };

class CPPLexer {
public:
    CPPLexer(const char* input, size_t length) : m_input(input), m_length(length) {}

    bool lex(CPPLexMode mode = CPPLexMode::IgnoreComments);

    const Vector<CPPToken>& tokens() const { return m_tokens; }

private:
    size_t characters_remaining() const { return m_length - m_index; }

    bool input_starts_with(const StringView& view) const {
        if (characters_remaining() < view.size()) {
            return false;
        }

        return memcmp(m_input + m_index, view.data(), view.size()) == 0;
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

    void commit_token(CPPToken::Type type) {
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
    Vector<CPPToken> m_tokens;
    bool m_in_preprocessor { false };
    bool m_expecting_preprocessor_keyword { false };
    bool m_in_include { false };
};
}
