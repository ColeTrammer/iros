#pragma once

namespace LIIM {

#ifdef __cpp_concepts
template<typename T>
concept InlineLinkedListNode = requires(T a) {
    T* h = a.next();
    T* t = a.prev();
    a.set_next(h);
    a.set_prev(t);
};
#endif /* __cpp_concepts */

template<typename T>
#ifdef __cpp_concepts
requires InlineLinkedListNode<T> class InlineLinkedList {
#else
class InlineLinkedList {
#endif /* __cpp_concepts */
public:
    InlineLinkedList() {}
    ~InlineLinkedList() { clear(); }

    void clear() {
        while (T* a = m_head; a) {
            a = a->next();
            delete a;
        }
    }

private:
    T* m_head { nullptr };
    T* m_tail { nullptr };
    size_t m_size { 0 };
};

}

using LIIM::InlineLinkedList;
using LIIM::InlineLinkedListNode;
