#pragma once

#include <di/container/iterator/prelude.h>
#include <di/util/addressof.h>
#include <di/util/exchange.h>
#include <di/util/immovable.h>
#include <di/vocab/optional/prelude.h>

namespace di::container {
struct DefaultIntrusiveForwardListTag {};

template<typename T, typename Tag = DefaultIntrusiveForwardListTag>
class IntrusiveForwardList;

template<typename Tag = DefaultIntrusiveForwardListTag>
struct IntrusiveForwardListElement : util::Immovable {
    constexpr IntrusiveForwardListElement() : next(nullptr) {}

    template<concepts::DerivedFrom<IntrusiveForwardListElement> T>
    constexpr T& down_cast() {
        return static_cast<T&>(*this);
    }

private:
    template<typename, typename>
    friend class IntrusiveForwardList;

    constexpr IntrusiveForwardListElement(IntrusiveForwardListElement* node) : next(node) {}

    IntrusiveForwardListElement* next;
};

template<typename T, typename Tag>
class IntrusiveForwardList {
private:
    using Node = IntrusiveForwardListElement<Tag>;

    struct Iterator : IteratorBase<Iterator, ForwardIteratorTag, T, ssize_t> {
    private:
        friend class IntrusiveForwardList;

        constexpr explicit Iterator(Node* node) : m_node(node) {}

    public:
        Iterator() = default;

        constexpr decltype(auto) operator*() const { return m_node->template down_cast<T>(); }

        constexpr void advance_one() { m_node = m_node->next; }

    private:
        constexpr friend bool operator==(Iterator const& a, Iterator const& b) { return a.m_node == b.m_node; }

        constexpr Node* node() const { return m_node; }

        Node* m_node { nullptr };
    };

    using ConstIterator = meta::ConstIterator<Iterator>;

public:
    constexpr IntrusiveForwardList() { reset_tail(); }

    IntrusiveForwardList(IntrusiveForwardList const&) = delete;

    constexpr IntrusiveForwardList(IntrusiveForwardList&& other) { *this = util::move(other); }

    IntrusiveForwardList& operator=(IntrusiveForwardList const&) = delete;

    constexpr IntrusiveForwardList& operator=(IntrusiveForwardList&& other) {
        m_head.next = util::exchange(other.m_head.next, nullptr);
        m_tail = other.m_tail;
        if (empty()) {
            reset_tail();
        }
        other.reset_tail();
        return *this;
    }

    ~IntrusiveForwardList() = default;

    constexpr bool empty() const { return !head(); }

    constexpr Iterator before_begin() { return Iterator(util::addressof(m_head)); }
    constexpr ConstIterator before_begin() const { return cbefore_begin(); }
    constexpr ConstIterator cbefore_begin() const { return Iterator(const_cast<Node*>(util::addressof(m_head))); }

    constexpr Iterator begin() { return Iterator(head()); }
    constexpr Iterator end() { return Iterator(); }

    constexpr ConstIterator begin() const { return Iterator(head()); }
    constexpr ConstIterator end() const { return Iterator(); }

    constexpr auto front() {
        return lift_bool(!empty()) % [&] {
            return util::ref(*begin());
        };
    }
    constexpr auto front() const {
        return lift_bool(!empty()) % [&] {
            return util::ref(*begin());
        };
    }

    constexpr void push_back(T& value) {
        auto* node = static_cast<Node*>(util::addressof(value));
        *m_tail = node;
        m_tail = util::addressof(node->next);
        node->next = nullptr;
    }

    constexpr void push_front(T& value) {
        auto* node = static_cast<Node*>(util::addressof(value));
        node->next = head();
        set_head(node);
    }

    constexpr Optional<T&> pop_front() {
        return lift_bool(!empty()) % [&] {
            auto* front = head();
            set_head(util::exchange(front->next, nullptr));
            if (util::addressof(front->next) == m_tail) {
                reset_tail();
            }
            return util::ref(front->template down_cast<T>());
        };
    }

    constexpr void prepend_container(IntrusiveForwardList&& other) {
        if (!other.empty()) {
            **other.m_tail = this->head();
            this->set_head(other.head());

            other.clear();
        }
    }

    constexpr void append_container(IntrusiveForwardList&& other) {
        if (!other.empty()) {
            auto* node = other.head();
            *m_tail = node;
            m_tail = util::addressof(node->next);
            other.clear();
        }
    }

    constexpr void clear() {
        set_head(nullptr);
        reset_tail();
    }

    constexpr Iterator insert_after(ConstIterator position, T& value) {
        auto* node = static_cast<Node*>(util::addressof(value));
        auto* prev = position.base().node();
        if (!prev || util::addressof(prev->next) == m_tail) {
            push_back(value);
        } else {
            node->next = prev->next;
            prev->next = node;
        }

        return Iterator(node);
    }

    constexpr Iterator erase_after(ConstIterator position) {
        if (!position.node()) {
            return end();
        }
        return erase_after(position, container::next(position));
    }
    constexpr Iterator erase_after(ConstIterator first, ConstIterator last) {
        if (first == last) {
            return last;
        }

        auto* prev = first.base().node();
        auto* end = last.base().node();
        if (!end) {
            m_tail = util::addressof(prev->next);
        }
        prev->next = end;
        return last.base();
    }

private:
    constexpr Node* head() const { return m_head.next; }
    constexpr void set_head(Node* head) { m_head.next = head; }

    constexpr void reset_tail() { m_tail = util::addressof(m_head.next); }

    Node m_head;
    Node** m_tail;
};
}