#pragma once

#include <liim/stack.h>
#include <liim/vector.h>

#include "generic_token.h"

template<typename TokenType, typename Value> class GenericParser {
public:
    using Token = GenericToken<TokenType, Value>;

    GenericParser(const Vector<Token>& tokens) : m_tokens(tokens) { m_state_stack.push(0); }
    virtual ~GenericParser() {}

    virtual bool parse() = 0;

protected:
    const Value& peek_value_stack() const { return m_value_stack.peek(); };

    void push_value_stack(const Value& value) { m_value_stack.push(value); }

    void consume_token() {
        m_value_stack.push(m_tokens[m_position].value());
        m_position++;
    }

    int current_state() const { return m_state_stack.peek(); }
    void push_state_stack(int state) { m_state_stack.push(state); }
    Value& pop_stack_state() {
        m_state_stack.pop();
        return m_value_stack.pop();
    }

    void jump_to(int state) {
        m_in_reduce = false;
        push_state_stack(state);
    }

    void reduce(TokenType type) {
        m_in_reduce = true;
        m_reduce_type = type;
    }

    TokenType peek_token_type() {
        if (m_in_reduce) {
            return m_reduce_type;
        }

        if (m_position >= m_tokens.size()) {
            return TokenType::End;
        }

        return m_tokens[m_position].type();
    }

private:
    Vector<Token> m_tokens;
    Stack<Value> m_value_stack;
    Stack<int> m_state_stack;
    int m_position { 0 };
    bool m_in_reduce { false };
    TokenType m_reduce_type { TokenType::End };
};