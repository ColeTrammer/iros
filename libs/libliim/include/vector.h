#pragma once

#include <assert.h>
#include <stdlib.h>
#include <string.h>

namespace LIIM {

template <typename T>
class Vector {
public:
    Vector(int capacity = 20)
        : m_capacity(capacity)
    {
        allocate_vector();
    }

    ~Vector()
    {
        for (int i = 0; i < m_size; i++) {
            get(i).~T();
        }
        delete[] m_vector;
    }

    int size() const { return m_size; }
    int capacity() const { return m_capacity; }

    void add(T t)
    {
        if (m_size >= m_capacity) {
            increase_capacity();
            allocate_vector();
        }

        m_vector[m_size++] = t;
    }

    void remove_last()
    {
        assert(m_size > 0);
        get(m_size - 1).~T();
        m_size--;
    }

    T& get(int i)
    {
        assert(i >= 0 && i < m_size);
        return m_vector[i];
    }

    const T& get(int i) const
    {
        assert(i >= 0 && i < m_size);
        return m_vector[i];
    }

    T& get_or(int i, T& val)
    {
        if (i < 0 || i >= m_size) {
            return val;
        }

        return get(i);
    }

    const T& get_or(int i, const T& val) const
    {
        if (i < 0 || i >= m_size) {
            return val;
        }

        return get(i);
    }

    T& operator[](int i) { return get(i); }
    const T& operator[](int i) const { return get(i); }

    T& first() { return get(0); }
    const T& first() const { return get(0); }

    T& last() { return get(m_size - 1); }
    const T& last() const { return get(m_size - 1); }

private:
    void increase_capacity()
    {
        m_capacity *= 2;
    }

    void allocate_vector()
    {
        T* replacement = static_cast<T*>(::operator new(m_capacity * sizeof(T)));
        if (!m_vector) {
            m_vector = replacement;
            return;
        }

        for (int i = 0; i < m_size; i++) {
            replacement[i] = get(i);
            get(i).~T();
        }

        m_vector = replacement;
    }

    int m_capacity;
    int m_size { 0 };
    T* m_vector { nullptr };
};

}