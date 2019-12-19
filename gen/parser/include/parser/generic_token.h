#pragma once

template<typename TokenType, typename Value> class GenericToken {
public:
    GenericToken(TokenType type, const Value& value) : m_type(type), m_value(value) {}
    ~GenericToken() {}

    TokenType type() const { return m_type; }

    Value& value() { return m_value; }
    const Value& value() const { return m_value; }

private:
    TokenType m_type;
    Value m_value;
};