#pragma once

#include <di/container/algorithm/compare.h>
#include <di/container/algorithm/equal.h>
#include <di/container/intrusive/list_node.h>
#include <di/container/iterator/const_iterator_impl.h>
#include <di/container/iterator/prelude.h>
#include <di/util/addressof.h>
#include <di/util/exchange.h>
#include <di/util/movable_box.h>
#include <di/vocab/optional/prelude.h>

namespace di::container {
template<typename Self>
struct IntrusiveListTag : IntrusiveTagBase<IntrusiveListNode<Self>> {};

struct DefaultIntrusiveListTag : IntrusiveListTag<DefaultIntrusiveListTag> {};

template<typename T, typename Tag, typename Self>
class IntrusiveList {
private:
    using Node = IntrusiveListNode<Tag>;
    using ConcreteNode = decltype(Tag::node_type(in_place_type<T>));

    constexpr static bool is_sized = Tag::is_sized(in_place_type<T>);

    constexpr decltype(auto) down_cast_self() {
        if constexpr (concepts::SameAs<Void, Self>) {
            return *this;
        } else {
            return static_cast<Self&>(*this);
        }
    }

    struct Iterator : IteratorBase<Iterator, BidirectionalIteratorTag, T, isize> {
    private:
        friend class IntrusiveList;

    public:
        Iterator() = default;

        constexpr Iterator(Node* node) : m_node(node) {}
        constexpr Iterator(Node& node) : m_node(util::addressof(node)) {}

        constexpr T& operator*() const { return Tag::down_cast(in_place_type<T>, static_cast<ConcreteNode&>(*m_node)); }
        constexpr T* operator->() const { return util::addressof(**this); }

        constexpr void advance_one() { m_node = m_node->next; }
        constexpr void back_one() { m_node = m_node->prev; }

    private:
        constexpr friend bool operator==(Iterator const& a, Iterator const& b) { return a.m_node == b.m_node; }

        constexpr Node* node() const { return m_node; }

        Node* m_node { nullptr };
    };

    using ConstIterator = container::ConstIteratorImpl<Iterator>;

public:
    constexpr IntrusiveList() { reset_head(); }

    IntrusiveList(IntrusiveList const&) = delete;

    constexpr IntrusiveList(IntrusiveList&& other) : IntrusiveList() { *this = util::move(other); }

    IntrusiveList& operator=(IntrusiveList const&) = delete;

    constexpr IntrusiveList& operator=(IntrusiveList&& other) {
        if (this != &other) {
            clear();
            m_head.value().next = other.m_head.value().next;
            m_head.value().prev = other.m_head.value().prev;

            // Update references to the new dummy head.
            m_head.value().next->prev = util::addressof(m_head.value());
            m_head.value().prev->next = util::addressof(m_head.value());

            if constexpr (is_sized) {
                m_size.value = other.size();
            }

            other.reset_head();
        }
        return *this;
    }

    constexpr ~IntrusiveList() { clear(); }

    constexpr bool empty() const { return head() == util::addressof(m_head.value()); }
    constexpr usize size() const
    requires(is_sized)
    {
        return m_size.value;
    }
    constexpr usize max_size() const { return math::NumericLimits<usize>::max; }

    constexpr Iterator begin() { return Iterator(head()); }
    constexpr Iterator end() { return Iterator(util::addressof(m_head.value())); }

    constexpr Iterator begin() const { return Iterator(head()); }
    constexpr Iterator end() const { return Iterator(const_cast<Node*>(util::addressof(m_head.value()))); }

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

    constexpr void push_back(Node& node) { insert(end(), node); }
    constexpr void push_front(Node& node) { insert(begin(), node); }

    constexpr auto pop_front() {
        return lift_bool(!empty()) % [&] {
            auto it = begin();
            erase(it);
            return util::ref(*it);
        };
    }

    constexpr auto pop_back() {
        return lift_bool(!empty()) % [&] {
            auto it = --end();
            erase(it);
            return util::ref(*it);
        };
    }

    constexpr void clear() { erase(begin(), end()); }

    constexpr Iterator insert(ConstIterator position, Node& node_ref) {
        auto* node = util::addressof(node_ref);
        auto* next = position.base().node();
        auto* prev = next->prev;

        node->next = next;
        node->prev = prev;

        prev->next = node;
        next->prev = node;

        if constexpr (is_sized) {
            ++m_size.value;
        }

        Tag::did_insert(down_cast_self(), static_cast<ConcreteNode&>(node_ref));

        return Iterator(node);
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

        for (auto it = first; it != last;) {
            auto save = it++;
            Tag::did_remove(down_cast_self(), static_cast<ConcreteNode&>(*save.base().node()));

            if constexpr (is_sized) {
                --m_size.value;
            }
        }

        return last.base();
    }

    constexpr void splice(ConstIterator position, IntrusiveList& other) { splice(position, util::move(other)); }
    constexpr void splice(ConstIterator position, IntrusiveList&& other) {
        splice(position, other, other.begin(), other.end());
    }

    constexpr void splice(ConstIterator position, IntrusiveList& other, ConstIterator it) {
        splice(position, util::move(other), it);
    }
    constexpr void splice(ConstIterator position, IntrusiveList&& other, ConstIterator it) {
        splice(position, other, it, container::next(it, 1));
    }

    constexpr void splice(ConstIterator position, IntrusiveList& other, ConstIterator first, ConstIterator last) {
        splice(position, util::move(other), first, last);
    }
    constexpr void splice(ConstIterator position, IntrusiveList&& other, ConstIterator first, ConstIterator last) {
        auto* first_node = first.base().node();
        auto* last_node = last.base().node();
        if (first_node == last_node) {
            return;
        }

        // Unlink from other.
        auto* inclusive_last_node = last_node->prev;
        {
            auto* prev_node = first_node->prev;
            prev_node->next = last_node;
            last_node->prev = prev_node;
        }

        // Relink with this.
        {
            auto* position_node = position.base().node();
            auto* prev_node = position_node->prev;

            prev_node->next = first_node;
            first_node->prev = prev_node;

            position_node->prev = inclusive_last_node;
            inclusive_last_node->next = position_node;
        }

        // Adjust size.
        if constexpr (is_sized) {
            if (this != &other) {
                auto size = 1 + container::distance(first, ConstIterator(Iterator(inclusive_last_node)));
                this->m_size.value += size;
                other.m_size.value -= size;
            }
        }
    }

private:
    constexpr friend bool operator==(IntrusiveList const& a, IntrusiveList const& b)
    requires(concepts::EqualityComparable<T>)
    {
        return container::equal(a, b);
    }

    constexpr friend auto operator<=>(IntrusiveList const& a, IntrusiveList const& b)
    requires(concepts::ThreeWayComparable<T>)
    {
        return container::compare(a, b);
    }

    constexpr Node* head() const { return m_head.value().next; }
    constexpr void set_head(Node* head) { m_head.value().next = head; }

    constexpr void reset_head() {
        m_head.value().next = m_head.value().prev = util::addressof(m_head.value());
        if constexpr (is_sized) {
            m_size.value = 0;
        }
    }

    util::MovableBox<Node> m_head;
    [[no_unique_address]] util::StoreIf<usize, is_sized> m_size { 0 };
};
}

namespace di {
using container::IntrusiveList;
using container::IntrusiveListTag;
}
