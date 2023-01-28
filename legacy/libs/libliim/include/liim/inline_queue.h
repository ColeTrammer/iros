#pragma once

#include <assert.h>
#include <stddef.h>

namespace LIIM {
template<typename T>
class InlineLinkedListNode {
public:
    T*& next() { return m_next; }
    const T*& next() const { return m_next; }

private:
    T* m_next { nullptr };
};

template<typename T>
class InlineQueue {
public:
    InlineQueue() {}
    InlineQueue(const InlineQueue<T>& other) { copy(other); }
    InlineQueue(InlineQueue<T>&& other) : m_head(other.m_head), m_tail(other.m_tail), m_size(other.m_size) {
        if (other.m_tail == &other.m_head) {
            m_tail = &m_head;
        }

        other.m_size = 0;
        other.m_head = nullptr;
        other.m_tail = &other.m_head;
    }

    InlineQueue<T>& operator==(const InlineQueue<T>& other) {
        clear();
        copy(other);
        return *this;
    }

    ~InlineQueue() { clear(); }

    T* head() { return m_head; }
    const T* head() const { return m_head; }

    T* tail() { return *m_tail; }
    const T* tail() const { return *m_tail; }

    bool empty() const { return !m_head; }

    size_t size() const { return m_size; }

    void clear() {
        while (m_head) {
            auto* next = m_head->next();
            delete m_head;
            m_head = next;
        }

        m_head = nullptr;
        m_tail = &m_head;
        m_size = 0;
    }

    void add(const T& item) {
        *m_tail = new T(item);
        m_tail = &(*m_tail)->next();
        m_size++;
    }

    T take_one() {
        assert(!empty());
        T to_return(*head());
        to_return.next() = nullptr;

        T* next = head()->next();
        delete m_head;
        m_head = next;

        if (m_head == nullptr) {
            m_tail = &m_head;
        }

        m_size--;
        return to_return;
    }

private:
    void copy(const InlineQueue<T>& other) {
        T* start = const_cast<T*>(other.head());
        T** iter = m_tail;
        while (start) {
            *iter = new T(*start);
            iter = &(*iter)->next();
            m_size++;
            start = start->next();
        }

        m_tail = iter;
    }

    T* m_head { nullptr };
    T** m_tail { &m_head };
    size_t m_size { 0 };
};
}

using LIIM::InlineLinkedListNode;
using LIIM::InlineQueue;