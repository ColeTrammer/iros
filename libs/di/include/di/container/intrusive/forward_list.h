#pragma once

#include <di/container/intrusive/forward_list_forward_declaration.h>
#include <di/container/intrusive/forward_list_node.h>
#include <di/container/iterator/prelude.h>
#include <di/util/addressof.h>
#include <di/util/exchange.h>
#include <di/util/immovable.h>
#include <di/util/movable_box.h>
#include <di/vocab/optional/prelude.h>

namespace di::container {
template<typename Self>
struct IntrusiveForwardListTag : IntrusiveTagBase<IntrusiveForwardListNode<Self>> {};

struct DefaultIntrusiveForwardListTag : IntrusiveForwardListTag<DefaultIntrusiveForwardListTag> {};

template<typename T, typename Tag, typename Self>
class IntrusiveForwardList {
private:
    using Node = IntrusiveForwardListNode<Tag>;
    using ConcreteNode = decltype(Tag::node_type(in_place_type<T>));

    constexpr static bool is_sized = Tag::is_sized(in_place_type<T>);
    constexpr static bool store_tail = Tag::always_store_tail(in_place_type<T>);

    constexpr decltype(auto) down_cast_self() {
        if constexpr (concepts::SameAs<Void, Self>) {
            return *this;
        } else {
            return static_cast<Self&>(*this);
        }
    }

    struct Iterator : IteratorBase<Iterator, ForwardIteratorTag, T, ssize_t> {
    private:
        friend class IntrusiveForwardList;

    public:
        Iterator() = default;

        constexpr explicit Iterator(Node* node) : m_node(node) {}
        constexpr explicit Iterator(Node& node) : m_node(util::addressof(node)) {}

        constexpr T& operator*() const { return Tag::down_cast(in_place_type<T>, static_cast<ConcreteNode&>(*m_node)); }
        constexpr T* operator->() const { return util::addressof(**this); }

        constexpr void advance_one() { m_node = m_node->next; }

        constexpr Node* node() const { return m_node; }

    private:
        constexpr friend bool operator==(Iterator const& a, Iterator const& b) { return a.m_node == b.m_node; }

        Node* m_node { nullptr };
    };

    using ConstIterator = meta::ConstIterator<Iterator>;

public:
    constexpr IntrusiveForwardList() { reset_tail(); }

    IntrusiveForwardList(IntrusiveForwardList const&) = delete;

    constexpr IntrusiveForwardList(IntrusiveForwardList&& other) : IntrusiveForwardList() { *this = util::move(other); }

    IntrusiveForwardList& operator=(IntrusiveForwardList const&) = delete;

    constexpr IntrusiveForwardList& operator=(IntrusiveForwardList&& other) {
        m_head.value().next = util::exchange(other.m_head.value().next, nullptr);

        if constexpr (is_sized) {
            m_size.value = util::exchange(other.m_size.value, 0);
        }

        if constexpr (store_tail) {
            if (!empty()) {
                m_tail.value = other.m_tail.value;
            }
            other.reset_tail();
        }
        return *this;
    }

    constexpr ~IntrusiveForwardList() { clear(); }

    constexpr bool empty() const { return !head(); }
    constexpr usize size() const
    requires(is_sized)
    {
        return m_size.value;
    }
    constexpr usize max_size() const { return math::NumericLimits<usize>::max; }

    constexpr Iterator before_begin() { return Iterator(util::addressof(m_head.value())); }
    constexpr ConstIterator before_begin() const {
        return Iterator(const_cast<Node*>(util::addressof(m_head.value())));
    }

    constexpr Iterator begin() { return Iterator(head()); }
    constexpr Iterator end() { return Iterator(); }

    constexpr ConstIterator begin() const { return Iterator(head()); }
    constexpr ConstIterator end() const { return Iterator(); }

    constexpr Iterator before_end()
    requires(store_tail)
    {
        return Iterator(m_tail.value);
    }
    constexpr ConstIterator before_end() const
    requires(store_tail)
    {
        return Iterator(m_tail.value);
    }

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

    constexpr auto back()
    requires(store_tail)
    {
        return lift_bool(!empty()) % [&] {
            return util::ref(*before_end());
        };
    }
    constexpr auto back() const
    requires(store_tail)
    {
        return lift_bool(!empty()) % [&] {
            return util::cref(*before_end());
        };
    }

    constexpr void push_front(Node& node) { insert_after(before_begin(), node); }
    constexpr void push_back(Node& node)
    requires(store_tail)
    {
        insert_after(before_end(), node);
    }

    constexpr Optional<T&> pop_front() {
        return lift_bool(!empty()) % [&] {
            auto it = begin();
            erase_after(before_begin());
            if (empty()) {
                reset_tail();
            }
            return util::ref(*it);
        };
    }

    constexpr void clear() { erase_after(before_begin(), end()); }

    constexpr Iterator insert_after(ConstIterator position, Node& node_ref) {
        auto* node = util::addressof(node_ref);
        auto* prev = position.base().node();

        DI_ASSERT(prev);
        if constexpr (store_tail) {
            if (prev == m_tail.value) {
                m_tail.value = node;
            }
        }

        node->next = prev->next;
        prev->next = node;

        if constexpr (is_sized) {
            ++m_size.value;
        }

        Tag::did_insert(down_cast_self(), static_cast<ConcreteNode&>(node_ref));

        return Iterator(node);
    }

    constexpr Iterator erase_after(ConstIterator position) {
        if (!position.base().node()) {
            return end();
        }
        auto next = container::next(position);
        if (!next.base().node()) {
            return end();
        }
        return erase_after(position, container::next(next));
    }
    constexpr Iterator erase_after(ConstIterator first, ConstIterator last) {
        if (first == last) {
            return last.base();
        }

        auto* prev = first.base().node();
        auto* end = last.base().node();
        if constexpr (store_tail) {
            if (!end) {
                m_tail.value = prev;
            }
        }
        prev->next = end;

        for (auto it = ++first; it != last;) {
            auto save = it++;
            Tag::did_remove(down_cast_self(), static_cast<ConcreteNode&>(*save.base().node()));

            if constexpr (is_sized) {
                --m_size.value;
            }
        }
        return last.base();
    }

private:
    constexpr Node* head() const { return m_head.value().next; }
    constexpr void set_head(Node* head) { m_head.value().next = head; }

    constexpr void reset_tail() {
        if constexpr (store_tail) {
            m_tail.value = util::addressof(m_head.value());
        }
    }

    util::MovableBox<Node> m_head;
    [[no_unique_address]] util::StoreIf<Node*, store_tail> m_tail { nullptr };
    [[no_unique_address]] util::StoreIf<usize, is_sized> m_size { 0 };
};
}
