#pragma once

#include <assert.h>
#include <liim/traits.h>
#include <new>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace LIIM {

template<typename T> class Vector {
public:
    explicit Vector(int capacity = 20)
        : m_capacity(capacity) {
        allocate_vector();
    }

    Vector(const Vector& to_copy)
        : m_capacity(to_copy.capacity()) {
        allocate_vector();
        m_size = to_copy.size();
        if (m_size == 0) {
            return;
        }
        if constexpr (Traits<T>::is_simple()) {
            memmove(m_vector, to_copy.m_vector, sizeof(T) * m_size);
        } else {
            for (int i = 0; i < m_size; i++) {
                new (&m_vector[i]) T(to_copy.m_vector[i]);
            }
        }
    }

    Vector(const T* buffer, int num_elements)
        : m_capacity(num_elements)
        , m_size(num_elements) {
        assert(buffer);
        allocate_vector();
        if constexpr (Traits<T>::is_simple()) {
            memmove(m_vector, buffer, sizeof(T) * num_elements);
        } else {
            for (int i = 0; i < m_size; i++) {
                new (&m_vector[i]) T(buffer[i]);
            }
        }
    }

    ~Vector() {
        for (int i = 0; i < m_size; i++) {
            get(i).~T();
        }
        m_size = 0;
        if (m_vector) {
            free(m_vector);
        }
        m_vector = nullptr;
    }

    int size() const {
        return m_size;
    }
    int capacity() const {
        return m_capacity;
    }

    void add(const T& t) {
        if (m_size >= m_capacity) {
            increase_capacity();
            allocate_vector();
        }

        new (&m_vector[m_size++]) T(t);
    }

    void remove_last() {
        assert(m_size > 0);
        get(m_size - 1).~T();
        m_size--;
    }

    T& get(int i) {
        assert(i >= 0 && i < m_size);
        return m_vector[i];
    }

    const T& get(int i) const {
        assert(i >= 0 && i < m_size);
        return m_vector[i];
    }

    T& get_or(int i, T& val) {
        if (i < 0 || i >= m_size) {
            return val;
        }

        return get(i);
    }

    const T& get_or(int i, const T& val) const {
        if (i < 0 || i >= m_size) {
            return val;
        }

        return get(i);
    }

    T& operator[](int i) {
        return get(i);
    }
    const T& operator[](int i) const {
        return get(i);
    }

    T& first() {
        return get(0);
    }
    const T& first() const {
        return get(0);
    }

    T& last() {
        return get(m_size - 1);
    }
    const T& last() const {
        return get(m_size - 1);
    }

    T* vector() {
        return m_vector;
    }
    const T* vector() const {
        return m_vector;
    }

    template<typename C> void for_each(C callback) {
        for (int i = 0; i < size(); i++) {
            callback(get(i));
        }
    }

    template<typename C> void for_each_reverse(C callback) {
        for (int i = size() - 1; i >= 0; i--) {
            callback(get(i));
        }
    }

    void remove_element(const T& val) {
        for (int i = size() - 1; i >= 0; i--) {
            if (get(i) == val) {
                get(i).~T();
                if (i != size() - 1) {
                    if constexpr (Traits<T>::is_simple()) {
                        memmove(m_vector + i, m_vector + i + 1, sizeof(T) * (size() - i - 1));
                    } else {
                        for (int j = i; j < size() - 1; j++) {
                            new (m_vector + j) T(get(j + 1));
                            get(j + 1).~T();
                        }
                    }
                }

                m_size--;
            }
        }
    }

    template<typename C> void remove_if(C callback) {
        for_each_reverse([&](auto& elem) {
            if (callback(elem)) {
                remove_element(elem);
            }
        });
    }

private:
    void increase_capacity() {
        m_capacity *= 2;
    }

    void allocate_vector() {
        T* replacement = static_cast<T*>(malloc(m_capacity * sizeof(T)));
        if (!m_vector) {
            m_vector = replacement;
            return;
        }

        if constexpr (Traits<T>::is_simple()) {
            memmove(replacement, m_vector, sizeof(T) * m_size);
        } else {
            for (int i = 0; i < m_size; i++) {
                new (&replacement[i]) T(m_vector[i]);
                get(i).~T();
            }
        }

        m_vector = replacement;
    }

    int m_capacity;
    int m_size { 0 };
    T* m_vector { nullptr };
};

}

using LIIM::Vector;