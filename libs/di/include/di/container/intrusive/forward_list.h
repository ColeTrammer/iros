#pragma once

#include <di/container/interface/erase.h>
#include <di/container/intrusive/forward_list_forward_declaration.h>
#include <di/container/intrusive/forward_list_node.h>
#include <di/container/iterator/const_iterator_impl.h>
#include <di/container/iterator/next.h>
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
    using ConcreteSelf = meta::Conditional<SameAs<Void, Self>, IntrusiveForwardList, Self>;

    constexpr static bool is_sized = Tag::is_sized(in_place_type<T>);
    constexpr static bool store_tail = Tag::always_store_tail(in_place_type<T>);

    constexpr auto down_cast_self() -> decltype(auto) {
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

        constexpr auto operator*() const -> T& {
            return Tag::down_cast(in_place_type<T>, static_cast<ConcreteNode&>(*m_node));
        }
        constexpr auto operator->() const -> T* { return util::addressof(**this); }

        constexpr void advance_one() { m_node = m_node->next; }

        constexpr auto node() const -> Node* { return m_node; }

    private:
        constexpr friend auto operator==(Iterator const& a, Iterator const& b) -> bool { return a.m_node == b.m_node; }

        Node* m_node { nullptr };
    };

    using ConstIterator = container::ConstIteratorImpl<Iterator>;

public:
    constexpr IntrusiveForwardList() { reset_tail(); }

    IntrusiveForwardList(IntrusiveForwardList const&) = delete;

    constexpr IntrusiveForwardList(IntrusiveForwardList&& other) : IntrusiveForwardList() { *this = util::move(other); }

    auto operator=(IntrusiveForwardList const&) -> IntrusiveForwardList& = delete;

    constexpr auto operator=(IntrusiveForwardList&& other) -> IntrusiveForwardList& {
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

    constexpr auto empty() const -> bool { return !head(); }
    constexpr auto size() const -> usize
    requires(is_sized)
    {
        return m_size.value;
    }
    constexpr auto max_size() const -> usize { return math::NumericLimits<usize>::max; }

    constexpr auto before_begin() -> Iterator { return Iterator(util::addressof(m_head.value())); }
    constexpr auto before_begin() const -> ConstIterator {
        return Iterator(const_cast<Node*>(util::addressof(m_head.value())));
    }

    constexpr auto begin() { return Iterator(head()); }
    constexpr auto end() -> Iterator { return Iterator(); }

    constexpr auto begin() const { return Iterator(head()); }
    constexpr auto end() const -> ConstIterator { return Iterator(); }

    constexpr auto before_end() -> Iterator
    requires(store_tail)
    {
        return Iterator(m_tail.value);
    }
    constexpr auto before_end() const -> ConstIterator
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

    constexpr auto pop_front() -> Optional<T&> {
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

    constexpr auto insert_after(ConstIterator position, Node& node_ref) -> Iterator {
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

    constexpr auto erase_after(ConstIterator position) -> Iterator {
        if (!position.base().node()) {
            return end();
        }
        auto next = container::next(position);
        if (!next.base().node()) {
            return end();
        }
        return erase_after(position, container::next(next));
    }
    constexpr auto erase_after(ConstIterator first, ConstIterator last) -> Iterator {
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
    template<typename F>
    requires(concepts::Predicate<F&, T const&>)
    constexpr friend auto tag_invoke(di::Tag<erase_if>, ConcreteSelf& self, F&& function) -> usize {
        auto it = self.before_begin();
        auto jt = next(it);

        auto result = 0ZU;
        while (jt != self.end()) {
            if (function(*jt)) {
                jt = self.erase_after(it);
                ++result;
            } else {
                ++it;
                ++jt;
            }
        }
        return result;
    }

    constexpr auto head() const -> Node* { return m_head.value().next; }
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

namespace di {
using container::IntrusiveForwardList;
using container::IntrusiveForwardListTag;
}
