#include <clanguage/clexer.h>
#include <ctype.h>

namespace CLanguage {

bool CLexer::lex(CLexMode mode) {
    auto find_unescaped_char = [&](char c) {
        consume();

        bool prev_was_backslash = false;
        for (;;) {
            switch (peek()) {
                case '\\':
                    prev_was_backslash = !prev_was_backslash;
                    consume();
                    continue;
                case EOF:
                    return;
                default:
                    break;
            }

            if (peek() == c && !prev_was_backslash) {
                consume();
                return;
            }

            prev_was_backslash = false;
            consume();
        }
    };

    bool prev_was_backslash = false;
    for (;;) {
        if (peek() == EOF) {
            return true;
        }

        if (m_in_preprocessor) {
            if (peek() == '\\') {
                prev_was_backslash = !prev_was_backslash;
                if (prev_was_backslash && peek(1) == '\n') {
                    begin_token();
                    consume();
                    commit_token(CToken::Type::PreprocessorBackslash);
                    consume();
                    prev_was_backslash = false;
                    continue;
                }
                consume();
                continue;
            }

            if (peek() == '\n') {
                consume();
                m_in_preprocessor = false;
                continue;
            }

            if (peek() == '#' && peek(1) == '#') {
                begin_token();
                consume(2);
                commit_token(CToken::Type::PreprocessorPoundPound);
                continue;
            }

            if (peek() == '#') {
                begin_token();
                consume();
                commit_token(CToken::Type::PreprocessorPound);
                continue;
            }

            if (peek() == '<' && m_in_include) {
                begin_token();
                find_unescaped_char('>');
                commit_token(CToken::Type::PreprocessorSystemIncludeString);
                m_in_include = false;
                continue;
            }
        }

        if (peek() == '#' && m_current_col == 0) {
            begin_token();
            consume();
            commit_token(CToken::Type::PreprocessorStart);
            m_in_preprocessor = true;
            m_expecting_preprocessor_keyword = true;
            continue;
        }

        if (peek() == '\'') {
            begin_token();
            find_unescaped_char('\'');
            commit_token(CToken::Type::CharacterLiteral);
            continue;
        }

        if (peek() == '"') {
            begin_token();
            find_unescaped_char('"');
            commit_token(CToken::Type::StringLiteral);
            continue;
        }

        if (peek() == '/' && peek(1) == '/') {
            if (mode == CLexMode::IncludeComments) {
                begin_token();
            }
            find_unescaped_char('\n');
            if (mode == CLexMode::IncludeComments) {
                commit_token(CToken::Type::Comment);
            }
            continue;
        }

        if (peek() == '/' && peek(1) == '*') {
            if (mode == CLexMode::IncludeComments) {
                begin_token();
            }
            for (;;) {
                consume();
                if (peek() == EOF) {
                    return false;
                }
                if (peek() == '*' && peek(1) == '/') {
                    consume(2);
                    break;
                }
            }
            if (mode == CLexMode::IncludeComments) {
                commit_token(CToken::Type::Comment);
            }
            continue;
        }

        if (isdigit(peek())) {
            // FIXME: this is actually more complicated
            begin_token();
            while (isalnum(peek()) || peek() == '.' || peek() == '_') {
                consume();
            }
            commit_token(CToken::Type::NumericLiteral);
            continue;
        }

        if (isspace(peek())) {
            while (isspace(peek())) {
                consume();
            }
            continue;
        }

        if (m_in_preprocessor && m_expecting_preprocessor_keyword) {
#undef __ENUMERATE_C_PREPROCESSOR_KEYWORD
#define __ENUMERATE_C_PREPROCESSOR_KEYWORD(n, s)                                            \
    constexpr auto view_##n = StringView { s };                                             \
    if (isspace(peek(view_##n.size())) && input_starts_with(view_##n)) {                    \
        begin_token();                                                                      \
        consume(view_##n.size());                                                           \
        commit_token(CToken::Type::Preprocessor##n);                                        \
        m_expecting_preprocessor_keyword = false;                                           \
        if constexpr (CToken::Type::Preprocessor##n == CToken::Type::PreprocessorInclude) { \
            m_in_include = true;                                                            \
        }                                                                                   \
        continue;                                                                           \
    }
            __ENUMERATE_C_PREPROCESSOR_KEYWORDS
        }

        if (m_in_preprocessor && !isalnum(peek(7)) && peek(7) != '_' && peek(7) != '$' && peek(7) != EOF && input_starts_with("defined")) {
            begin_token();
            consume(7);
            commit_token(CToken::Type::PreprocessorDefined);
            continue;
        }

#undef __ENUMERATE_C_KEYWORD
#define __ENUMERATE_C_KEYWORD(n, s)                                                                                                        \
    constexpr auto view_##n = StringView { s };                                                                                            \
    if (!isalnum(peek(view_##n.size())) && peek(view_##n.size()) != '_' && peek(view_##n.size()) != '$' && peek(view_##n.size()) != EOF && \
        input_starts_with(view_##n)) {                                                                                                     \
        begin_token();                                                                                                                     \
        consume(view_##n.size());                                                                                                          \
        commit_token(CToken::Type::Keyword##n);                                                                                            \
        continue;                                                                                                                          \
    }
        __ENUMERATE_C_KEYWORDS

#undef __ENUMERATE_C_OP
#define __ENUMERATE_C_OP(n, s)                   \
    constexpr auto view_##n = StringView { s };  \
    if (input_starts_with(view_##n)) {           \
        begin_token();                           \
        consume(view_##n.size());                \
        commit_token(CToken::Type::Operator##n); \
        continue;                                \
    }
        __ENUMERATE_C_OPS

        if (isalpha(peek()) || peek() == '_' || peek() == '$') {
            begin_token();
            consume();
            while (isalnum(peek()) || peek() == '_' || peek() == '$') {
                consume();
            }
            commit_token(CToken::Type::Identifier);
            continue;
        }

        consume();
    }

    return false;
}

}
