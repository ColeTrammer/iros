#pragma once

namespace LIIM {

template<typename Left, typename Right> class Either {
public:
    Either(Left left) : m_type(Left), m_value({ .left = left }) {}
    Either(Right right) : m_type(Right), m_value(.right = right) {}

    enum class Type { Left, Right };

    union Value {
        Left left;
        Right right;
    };

    bool is_left() const { return m_type == Left; }
    bool is_right() const { return m_type == Right; }

    Left& left() { return m_value.left; }
    const Left& left() const { return m_value.left; }

    Left& right() { return m_value.right; }
    const Left& right() const { return m_value.right; }

private:
    Type m_type;
    Value m_value;
};

}

using LIIM::Either;