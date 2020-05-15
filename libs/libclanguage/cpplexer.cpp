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

    for (;;) {
        if (peek() == EOF) {
            return true;
        }

        if (peek() == '#' && m_current_col == 0) {
            find_unescaped_char('\n');
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

#undef __ENUMERATE_CPP_KEYWORD
#define __ENUMERATE_CPP_KEYWORD(n, s)                                                                                    \
    if (input_starts_with(StringView(s)) && !isalnum(peek(StringView(s).size())) && peek(StringView(s).size()) != '_' && \
        peek(StringView(s).size()) != '$' && peek(StringView(s).size()) != EOF) {                                        \
        begin_token();                                                                                                   \
        consume(StringView(s).size());                                                                                   \
        commit_token(CPPToken::Type::Keyword##n);                                                                        \
        continue;                                                                                                        \
    }
        __ENUMERATE_CPP_KEYWORDS

#undef __ENUMERATE_CPP_TYPE_TOKEN
#define __ENUMERATE_CPP_TYPE_TOKEN(n, s)                                                                                 \
    if (input_starts_with(StringView(s)) && !isalnum(peek(StringView(s).size())) && peek(StringView(s).size()) != '_' && \
        peek(StringView(s).size()) != '$' && peek(StringView(s).size()) != EOF) {                                        \
        begin_token();                                                                                                   \
        consume(StringView(s).size());                                                                                   \
        commit_token(CPPToken::Type::Type##n);                                                                           \
        continue;                                                                                                        \
    }
        __ENUMERATE_CPP_TYPE_TOKENS

#undef __ENUMERATE_CPP_LITERAL
#define __ENUMERATE_CPP_LITERAL(n, s)                                                                                    \
    if (input_starts_with(StringView(s)) && !isalnum(peek(StringView(s).size())) && peek(StringView(s).size()) != '_' && \
        peek(StringView(s).size()) != '$' && peek(StringView(s).size()) != EOF) {                                        \
        begin_token();                                                                                                   \
        consume(StringView(s).size());                                                                                   \
        commit_token(CPPToken::Type::Literal##n);                                                                        \
        continue;                                                                                                        \
    }
        __ENUMERATE_CPP_LITERALS

#undef __ENUMERATE_CPP_OP
#define __ENUMERATE_CPP_OP(n, s)                   \
    if (input_starts_with(StringView(s))) {        \
        begin_token();                             \
        consume(StringView(s).size());             \
        commit_token(CPPToken::Type::Operator##n); \
        continue;                                  \
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
