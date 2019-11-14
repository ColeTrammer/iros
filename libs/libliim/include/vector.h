#pragma once

#include <assert.h>
#include <stdlib.h>

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

    T& get(int i)
    {
        assert(i >= 0 && i < m_size);
        return m_vector[i];
    }

    const T& get(int i) const
    {
        return const_cast<Vector<T>>(this).get(i);
    }

    T& operator[](int i) { return get(i); }
    const T& at(int i) const { return get(i); }

private:
    void increase_capacity()
    {
        m_capacity *= 2;
    }

    void allocate_vector()
    {
        m_vector = static_cast<T*>(realloc(m_vector, m_capacity * sizeof(T)));
    }

    int m_capacity;
    int m_size { 0 };
    T* m_vector { nullptr };
};

}