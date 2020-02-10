#pragma once

#include <assert.h>
#include <liim/vector.h>

namespace LIIM {

template<typename T>
class Stack {
public:
    Stack() {}
    Stack(const Stack<T>& other) : m_vector(other.m_vector) {}

    ~Stack() {}

    T& peek() {
        assert(!empty());
        return m_vector.last();
    }
    const T& peek() const { return const_cast<Stack<T>&>(*this).peek(); }

    T pop() {
        assert(!empty());

        T val = m_vector.last();
        m_vector.remove_last();

        return val;
    }

    void push(const T& val) { m_vector.add(val); }

    bool empty() const { return m_vector.empty(); }

private:
    Vector<T> m_vector;
};

}

using LIIM::Stack;