#include <stdio.h>

#include "lexer.h"

Lexer::Lexer(char* buffer, size_t size) : m_buffer(buffer), m_size(size) {}

Lexer::~Lexer() {}

void Lexer::consume() {
    m_pos++;
}

void Lexer::commit_token(TokenType type) {
    m_vector.add({ type, { m_token_start, m_buffer + m_pos - 1 } });
    m_token_start = nullptr;
}

void Lexer::begin_token() {
    m_token_start = m_buffer + m_pos;
}

Vector<Token<TokenType>> Lexer::lex() {
    bool in_percent = false;
    bool in_token_decl = false;

    while (m_pos < m_size) {
        char start = m_buffer[m_pos];
        switch (start) {
            case '%':
                if (in_percent) {
                    consume();
                    commit_token(TokenType::TokenPercentPercent);
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
            case '\t':
            case ' ':
            case '\r':
            case '\n':
                if (m_token_start) {
                    commit_token(in_percent ? (in_token_decl ? TokenType::TokenTokenMarker : TokenType::TokenStartMarker)
                                            : TokenType::TokenWord);
                    in_percent = false;
                    in_token_decl = false;
                }
                consume();
                break;
            case '"':
            case '\'':
                consume();
                begin_token();
                while (m_buffer[m_pos] != start) {
                    consume();
                }
                commit_token(TokenType::TokenLiteral);
                consume();
                break;
            case '|':
                if (m_token_start) {
                    commit_token(TokenType::TokenWord);
                }
                begin_token();
                consume();
                commit_token(TokenType::TokenPipe);
                break;
            case '(':
            case '[':
                if (m_token_start) {
                    commit_token(TokenType::TokenWord);
                }
                begin_token();
                consume();
                commit_token(TokenType::TokenLeftParenthesis);
                break;
            case ')':
            case ']':
                if (m_token_start) {
                    commit_token(TokenType::TokenWord);
                }
                begin_token();
                consume();
                commit_token(TokenType::TokenRightParenthesis);
                break;
            case '+':
                if (m_token_start) {
                    commit_token(TokenType::TokenWord);
                }
                if (m_vector.last().type() != TokenType::TokenRightParenthesis) {
                    m_vector.insert({ TokenType::TokenLeftParenthesis, "(" }, m_vector.size() - 1);
                    m_vector.add({ TokenType::TokenRightParenthesis, ")" });
                }
                begin_token();
                consume();
                commit_token(TokenType::TokenPlus);
                break;
            case '*':
                if (m_token_start) {
                    commit_token(TokenType::TokenWord);
                }
                if (m_vector.last().type() != TokenType::TokenRightParenthesis) {
                    m_vector.insert({ TokenType::TokenLeftParenthesis, "(" }, m_vector.size() - 1);
                    m_vector.add({ TokenType::TokenRightParenthesis, ")" });
                }
                begin_token();
                consume();
                commit_token(TokenType::TokenStar);
                break;
            case '?':
                if (m_token_start) {
                    commit_token(TokenType::TokenWord);
                }
                if (m_vector.last().type() != TokenType::TokenRightParenthesis) {
                    m_vector.insert({ TokenType::TokenLeftParenthesis, "(" }, m_vector.size() - 1);
                    m_vector.add({ TokenType::TokenRightParenthesis, ")" });
                }
                begin_token();
                consume();
                commit_token(TokenType::TokenQuestionMark);
                break;
            case ':':
                if (m_token_start) {
                    commit_token(TokenType::TokenLhs);
                } else {
                    m_vector.last().set_type(TokenType::TokenLhs);
                }
                begin_token();
                consume();
                commit_token(TokenType::TokenColon);
                while (m_pos < m_size && (m_buffer[m_pos] == ':' || m_buffer[m_pos] == '=')) {
                    m_pos++;
                }
                break;
            case ';':
                if (m_token_start) {
                    commit_token(TokenType::TokenWord);
                }
                begin_token();
                consume();
                commit_token(TokenType::TokenSemicolon);
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

    if (m_token_start) {
        commit_token(TokenType::TokenWord);
    }
    m_vector.add({ TokenType::TokenEnd, "END" });
    return m_vector;
}
