#pragma once

#include <liim/vector.h>
#include <stddef.h>

#include "token.h"

class Lexer {
public:
    Lexer(char* buffer, size_t size);
    ~Lexer();

    Vector<Token> lex();

private:
    void consume();
    void begin_token();
    void commit_token(Token::Type type);

    char* m_buffer;
    char* m_token_start { nullptr };
    size_t m_size;
    size_t m_pos { 0 };
    Vector<Token> m_vector;
};