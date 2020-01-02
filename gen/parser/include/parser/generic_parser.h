#pragma once

#include <liim/stack.h>
#include <liim/string_view.h>
#include <liim/vector.h>
#include <parser/generic_lexer.h>
#include <parser/generic_token.h>

// #define GENERIC_PARSER_DEBUG

template<typename TokenType, typename Value> class GenericParser {
public:
    using Token = GenericToken<TokenType, Value>;

    GenericParser(GenericLexer<TokenType, Value>& lexer) : m_lexer(lexer) { m_state_stack.push(0); }
    virtual ~GenericParser() {}

    virtual bool is_valid_token_type_in_current_state_for_shift(TokenType) const { return false; }
    virtual bool is_valid_token_type_in_current_state(TokenType) const { return false; }
    virtual bool parse() = 0;

protected:
    const Value& peek_value_stack() const { return m_value_stack.peek(); };

    void push_value_stack(const Value& value) {
#ifdef GENERIC_PARSER_DEBUG
        fprintf(stderr, "Pushing value stack\n");
#endif /* GENERIC_PARSER_DEBUG */
        m_value_stack.push(value);
#ifdef GENERIC_PARSER_DEBUG
        fprintf(stderr, "Done pushing value stack\n");
#endif /* GENERIC_PARSER_DEBUG */
    }

    void consume_token() {
#ifdef GENERIC_PARSER_DEBUG
        fprintf(stderr, "Pushing value b/c consume token\n");
#endif /* GENERIC_PARSER_DEBUG */
        m_value_stack.push(m_lexer.peek_next_token_value());
#ifdef GENERIC_PARSER_DEBUG
        fprintf(stderr, "Done pushing value b/c consume token\n");
#endif /* GENERIC_PARSER_DEBUG */
        m_lexer.advance();
    }

    int current_state() const {
#ifdef GENERIC_PARSER_DEBUG
        fprintf(stderr, "Current state: %d\n", m_state_stack.peek());
#endif /* GENERIC_PARSER_DEBUG */
        return m_state_stack.peek();
    }
    void push_state_stack(int state) { m_state_stack.push(state); }
    Value pop_stack_state() {
        m_state_stack.pop();
#ifdef GENERIC_PARSER_DEBUG
        fprintf(stderr, "Popping value stack\n");
#endif /* GENERIC_PARSER_DEBUG */
        Value v = m_value_stack.pop();
#ifdef GENERIC_PARSER_DEBUG
        fprintf(stderr, "Done popping value stack\n");
#endif /* GENERIC_PARSER_DEBUG */
        return v;
    }

    void jump_to(int state) {
        m_in_reduce = false;
        push_state_stack(state);
    }

    void reduce(TokenType type) {
        m_in_reduce = true;
        m_reduce_type = type;
    }

    Value current_token() {
        if (m_lexer.peek_next_token_type() == TokenType::End) {
            return Value();
        }
        return m_lexer.peek_next_token_value();
    }

    TokenType peek_token_type() {
        if (m_in_reduce) {
            return m_reduce_type;
        }

        return m_lexer.peek_next_token_type();
    }

private:
    GenericLexer<TokenType, Value>& m_lexer;
    Stack<Value> m_value_stack;
    Stack<int> m_state_stack;
    int m_position { 0 };
    bool m_in_reduce { false };
    TokenType m_reduce_type { TokenType::End };
};