#include <clanguage/cpplexer.h>
#include <ctype.h>

namespace CLanguage {

bool CPPLexer::lex(CPPLexMode mode) {
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
                    commit_token(CPPToken::Type::PreprocessorBackslash);
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
                commit_token(CPPToken::Type::PreprocessorPoundPound);
                continue;
            }

            if (peek() == '#') {
                begin_token();
                consume();
                commit_token(CPPToken::Type::PreprocessorPound);
                continue;
            }

            if (peek() == '<' && m_in_include) {
                begin_token();
                find_unescaped_char('>');
                commit_token(CPPToken::Type::PreprocessorSystemIncludeString);
                m_in_include = false;
                continue;
            }
        }

        if (peek() == '#' && m_current_col == 0) {
            begin_token();
            consume();
            commit_token(CPPToken::Type::PreprocessorStart);
            m_in_preprocessor = true;
            m_expecting_preprocessor_keyword = true;
            continue;
        }

        if (peek() == '\'') {
            begin_token();
            find_unescaped_char('\'');
            commit_token(CPPToken::Type::CharacterLiteral);
            continue;
        }

        if (peek() == '"') {
            begin_token();
            find_unescaped_char('"');
            commit_token(CPPToken::Type::StringLiteral);
            continue;
        }

        if (peek() == '/' && peek(1) == '/') {
            if (mode == CPPLexMode::IncludeComments) {
                begin_token();
            }
            find_unescaped_char('\n');
            if (mode == CPPLexMode::IncludeComments) {
                commit_token(CPPToken::Type::Comment);
            }
            continue;
        }

        if (peek() == '/' && peek(1) == '*') {
            if (mode == CPPLexMode::IncludeComments) {
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
            if (mode == CPPLexMode::IncludeComments) {
                commit_token(CPPToken::Type::Comment);
            }
            continue;
        }

        if (isdigit(peek())) {
            // FIXME: this is actually more complicated
            begin_token();
            while (isalnum(peek()) || peek() == '.' || peek() == '_') {
                consume();
            }
            commit_token(CPPToken::Type::NumericLiteral);
            continue;
        }

        if (isspace(peek())) {
            while (isspace(peek())) {
                consume();
            }
            continue;
        }

        if (m_in_preprocessor && m_expecting_preprocessor_keyword) {
#undef __ENUMERATE_CPP_PREPROCESSOR_KEYWORD
#define __ENUMERATE_CPP_PREPROCESSOR_KEYWORD(n, s)                                              \
    constexpr auto view_##n = StringView { s };                                                 \
    if (input_starts_with(view_##n) && isspace(peek(view_##n.size()))) {                        \
        begin_token();                                                                          \
        consume(view_##n.size());                                                               \
        commit_token(CPPToken::Type::Preprocessor##n);                                          \
        m_expecting_preprocessor_keyword = false;                                               \
        if constexpr (CPPToken::Type::Preprocessor##n == CPPToken::Type::PreprocessorInclude) { \
            m_in_include = true;                                                                \
        }                                                                                       \
        continue;                                                                               \
    }
            __ENUMERATE_CPP_PREPROCESSOR_KEYWORDS
        }

        if (m_in_preprocessor && input_starts_with("defined") && !isalnum(peek(7)) && peek(7) != '_' && peek(7) != '$' && peek(7) != EOF) {
            begin_token();
            consume(7);
            commit_token(CPPToken::Type::PreprocessorDefined);
            continue;
        }

#undef __ENUMERATE_CPP_KEYWORD
#define __ENUMERATE_CPP_KEYWORD(n, s)                                                                                                     \
    constexpr auto view_##n = StringView { s };                                                                                           \
    if (input_starts_with(view_##n) && !isalnum(peek(view_##n.size())) && peek(view_##n.size()) != '_' && peek(view_##n.size()) != '$' && \
        peek(view_##n.size()) != EOF) {                                                                                                   \
        begin_token();                                                                                                                    \
        consume(view_##n.size());                                                                                                         \
        commit_token(CPPToken::Type::Keyword##n);                                                                                         \
        continue;                                                                                                                         \
    }
        __ENUMERATE_CPP_KEYWORDS

#undef __ENUMERATE_CPP_TYPE_TOKEN
#define __ENUMERATE_CPP_TYPE_TOKEN(n, s)                                                                                                  \
    constexpr auto view_##n = StringView { s };                                                                                           \
    if (input_starts_with(view_##n) && !isalnum(peek(view_##n.size())) && peek(view_##n.size()) != '_' && peek(view_##n.size()) != '$' && \
        peek(view_##n.size()) != EOF) {                                                                                                   \
        begin_token();                                                                                                                    \
        consume(view_##n.size());                                                                                                         \
        commit_token(CPPToken::Type::Type##n);                                                                                            \
        continue;                                                                                                                         \
    }
        __ENUMERATE_CPP_TYPE_TOKENS

#undef __ENUMERATE_CPP_LITERAL
#define __ENUMERATE_CPP_LITERAL(n, s)                                                                                                     \
    constexpr auto view_##n = StringView { s };                                                                                           \
    if (input_starts_with(view_##n) && !isalnum(peek(view_##n.size())) && peek(view_##n.size()) != '_' && peek(view_##n.size()) != '$' && \
        peek(view_##n.size()) != EOF) {                                                                                                   \
        begin_token();                                                                                                                    \
        consume(view_##n.size());                                                                                                         \
        commit_token(CPPToken::Type::Literal##n);                                                                                         \
        continue;                                                                                                                         \
    }
        __ENUMERATE_CPP_LITERALS

#undef __ENUMERATE_CPP_OP
#define __ENUMERATE_CPP_OP(n, s)                         \
    constexpr auto view_operator_##n = StringView { s }; \
    if (input_starts_with(view_operator_##n)) {          \
        begin_token();                                   \
        consume(view_operator_##n.size());               \
        commit_token(CPPToken::Type::Operator##n);       \
        continue;                                        \
    }
        __ENUMERATE_CPP_OPS

        if (isalpha(peek()) || peek() == '_' || peek() == '$') {
            begin_token();
            consume();
            while (isalnum(peek()) || peek() == '_' || peek() == '$') {
                consume();
            }
            commit_token(CPPToken::Type::Identifier);
            continue;
        }

        consume();
    }

    return false;
}
}
