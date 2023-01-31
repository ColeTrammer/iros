#pragma once

#include <di/container/iterator/prelude.h>
#include <di/util/addressof.h>
#include <di/util/exchange.h>
#include <di/util/immovable.h>
#include <di/vocab/optional/prelude.h>

namespace di::container {
struct DefaultIntrusiveListTag {};

template<typename T, typename Tag = DefaultIntrusiveListTag>
class IntrusiveList;

template<typename Tag = DefaultIntrusiveListTag>
struct IntrusiveListElement : util::Immovable {
    constexpr IntrusiveListElement() : next(nullptr), prev(nullptr) {}

    template<concepts::DerivedFrom<IntrusiveListElement> T>
    constexpr T& down_cast() {
        return static_cast<T&>(*this);
    }

private:
    template<typename, typename>
    friend class IntrusiveList;

    constexpr IntrusiveListElement(IntrusiveListElement* next_, IntrusiveListElement* prev_)
        : next(next_), prev(prev_) {}

    IntrusiveListElement* next;
    IntrusiveListElement* prev;
};

template<typename T, typename Tag>
class IntrusiveList {
private:
    using Node = IntrusiveListElement<Tag>;

    struct Iterator : IteratorBase<Iterator, BidirectionalIteratorTag, T, ssize_t> {
    private:
        friend class IntrusiveList;

        constexpr explicit Iterator(Node* node) : m_node(node) {}

    public:
        Iterator() = default;

        constexpr decltype(auto) operator*() const { return m_node->template down_cast<T>(); }

        constexpr void advance_one() { m_node = m_node->next; }
        constexpr void back_one() { m_node = m_node->prev; }

    private:
        constexpr friend bool operator==(Iterator const& a, Iterator const& b) { return a.m_node == b.m_node; }

        constexpr Node* node() const { return m_node; }

        Node* m_node { nullptr };
    };

    using ConstIterator = meta::ConstIterator<Iterator>;

public:
    constexpr IntrusiveList() { reset_head(); }

    IntrusiveList(IntrusiveList const&) = delete;

    constexpr IntrusiveList(IntrusiveList&& other) { *this = util::move(other); }

    IntrusiveList& operator=(IntrusiveList const&) = delete;

    constexpr IntrusiveList& operator=(IntrusiveList&& other) {
        m_head.next = util::exchange(other.m_head.next, util::addressof(other.m_head));
        m_head.prev = util::exchange(other.m_head.prev, util::addressof(other.m_head));
        if (empty()) {
            reset_head();
        }
        return *this;
    }

    ~IntrusiveList() = default;

    constexpr bool empty() const { return head() == util::addressof(m_head); }

    constexpr Iterator begin() { return Iterator(head()); }
    constexpr Iterator end() { return Iterator(util::addressof(m_head)); }

    constexpr Iterator begin() const { return Iterator(head()); }
    constexpr Iterator end() const { return Iterator(const_cast<Node*>(util::addressof(m_head))); }

    constexpr auto front() {
        return lift_bool(!empty()) % [&] {
            return util::ref(*begin());
        };
    }
    constexpr auto front() const {
        return lift_bool(!empty()) % [&] {
            return util::cref(*begin());
        };
    }

    constexpr auto back() {
        return lift_bool(!empty()) % [&] {
            return util::ref(*container::prev(end()));
        };
    }
    constexpr auto back() const {
        return lift_bool(!empty()) % [&] {
            return util::cref(*container::prev(end()));
        };
    }

    constexpr void push_back(T& value) { insert(end(), value); }
    constexpr void push_front(T& value) { insert(begin(), value); }

    constexpr auto pop_front() {
        return lift_bool(!empty()) % [&] {
            auto it = begin();
            erase(it);
            return util::ref(*it);
        };
    }

    constexpr void clear() { reset_head(); }

    constexpr Iterator insert(ConstIterator position, T& value) {
        auto* node = static_cast<Node*>(util::addressof(value));
        auto* next = position.base().node();
        auto* prev = next->prev;

        node->next = next;
        node->prev = prev;

        prev->next = node;
        next->prev = node;
        return Iterator(node);
    }

    constexpr Iterator erase(T& value) {
        auto* node = static_cast<Node*>(util::addressof(value));
        return erase(Iterator(node));
    }
    constexpr Iterator erase(ConstIterator position) { return erase(position, container::next(position)); }
    constexpr Iterator erase(ConstIterator first, ConstIterator last) {
        if (first == last) {
            return last.base();
        }

        auto* prev = first.base().node()->prev;
        auto* end = last.base().node();
        prev->next = end;
        end->prev = prev;
        return last.base();
    }

private:
    constexpr Node* head() const { return m_head.next; }
    constexpr void set_head(Node* head) { m_head.next = head; }

    constexpr void reset_head() { m_head.next = m_head.prev = util::addressof(m_head); }

    Node m_head;
};
}