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

#undef __ENUMERATE_C_KEYWORD
#define __ENUMERATE_C_KEYWORD(n, s)                                                                                      \
    if (input_starts_with(StringView(s)) && !isalnum(peek(StringView(s).size())) && peek(StringView(s).size()) != '_' && \
        peek(StringView(s).size()) != '$' && peek(StringView(s).size()) != EOF) {                                        \
        begin_token();                                                                                                   \
        consume(StringView(s).size());                                                                                   \
        commit_token(CToken::Type::Keyword##n);                                                                          \
        continue;                                                                                                        \
    }
        __ENUMERATE_C_KEYWORDS

#undef __ENUMERATE_C_OP
#define __ENUMERATE_C_OP(n, s)                   \
    if (input_starts_with(StringView(s))) {      \
        begin_token();                           \
        consume(StringView(s).size());           \
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
