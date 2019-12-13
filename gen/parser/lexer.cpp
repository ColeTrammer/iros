#include <stdio.h>

#include "lexer.h"

Lexer::Lexer(char* buffer, size_t size) : m_buffer(buffer), m_size(size) {}

Lexer::~Lexer() {}

void Lexer::consume() {
    m_pos++;
}

void Lexer::commit_token(Token::Type type) {
    m_vector.add({ type, { m_token_start, m_buffer + m_pos - 1 } });
    m_token_start = nullptr;
}

void Lexer::begin_token() {
    m_token_start = m_buffer + m_pos;
}

Vector<Token> Lexer::lex() {
    bool in_percent = false;
    bool in_token_decl = false;

    while (m_pos < m_size) {
        switch (m_buffer[m_pos]) {
            case '%':
                if (in_percent) {
                    consume();
                    commit_token(Token::Type::TokenPercentPercent);
                    in_percent = false;
                } else {
                    in_percent = true;
                    begin_token();
                    consume();
                }
                break;
            case '/':
                consume();
                if (m_buffer[m_pos] == '*') {
                    while (!(m_buffer[m_pos] == '*' && m_buffer[m_pos + 1] == '/')) {
                        consume();
                    }
                    consume();
                    consume();
                }
                break;
            case ' ':
            case '\r':
            case '\n':
                if (m_token_start) {
                    commit_token(in_percent ? (in_token_decl ? Token::Type::TokenTokenMarker : Token::Type::TokenStartMarker)
                                            : Token::Type::TokenWord);
                    in_percent = false;
                    in_token_decl = false;
                }
                consume();
                break;
            case '\'':
                consume();
                begin_token();
                while (m_buffer[m_pos] != '\'') {
                    consume();
                }
                commit_token(Token::Type::TokenLiteral);
                consume();
                break;
            case '|':
                if (m_token_start) {
                    commit_token(Token::Type::TokenWord);
                }
                begin_token();
                consume();
                commit_token(Token::Type::TokenPipe);
                break;
            case ':':
                if (m_token_start) {
                    commit_token(Token::Type::TokenWord);
                }
                begin_token();
                consume();
                commit_token(Token::Type::TokenColon);
                break;
            case ';':
                if (m_token_start) {
                    commit_token(Token::Type::TokenWord);
                }
                begin_token();
                consume();
                commit_token(Token::Type::TokenSemicolon);
                break;
            default:
                if (in_percent) {
                    if (m_buffer[m_pos] == 't') {
                        in_token_decl = true;
                    }

                    consume();
                    consume();
                    consume();
                    consume();
                    consume();
                    break;
                }

                if (!m_token_start) {
                    begin_token();
                    consume();
                    break;
                }

                consume();
                break;
        }
    }

    return m_vector;
}