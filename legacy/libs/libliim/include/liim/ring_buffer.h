#pragma once

#include <liim/utilities.h>

namespace LIIM {
template<typename T, size_t N>
class RingBuffer {
public:
    RingBuffer() : m_buffer(reinterpret_cast<T*>(malloc(N * sizeof(T)))) {}
    RingBuffer(const RingBuffer& other) = delete;
    RingBuffer(RingBuffer&& other) = delete;

    ~RingBuffer() {
        while (!empty()) {
            take();
        }
        free(m_buffer);
    }

    void clear() {
        m_head = 0;
        m_tail = 0;
        m_size = 0;
    }

    bool empty() const { return m_size == 0; }
    size_t size() const { return m_size; }
    constexpr size_t capacity() const { return N; }

    bool add(T object) {
        if (size() >= capacity()) {
            return false;
        }

        new (&m_buffer[m_tail++]) T(move(object));
        m_size++;
        m_tail %= capacity();
        return true;
    }

    void force_add(T object) {
        if (size() == capacity()) {
            take();
        }
        add(move(object));
    }

    T take() {
        assert(!empty());
        T object = move(m_buffer[m_head]);
        m_buffer[m_head++].~T();
        m_size--;
        m_head %= capacity();
        return object;
    }

    const T& access_from_head(size_t index) {
        assert(index < size());
        index += m_head;
        index %= capacity();
        return m_buffer[index];
    }

    const T& access_from_tail(size_t index) { return access_from_head(size() - index); }

private:
    T* m_buffer { nullptr };
    size_t m_head { 0 };
    size_t m_tail { 0 };
    size_t m_size { 0 };
};
}

using LIIM::RingBuffer;
