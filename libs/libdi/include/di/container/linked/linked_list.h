#pragma once

#include <di/container/algorithm/compare.h>
#include <di/container/algorithm/equal.h>
#include <di/container/allocator/allocator.h>
#include <di/container/concepts/prelude.h>
#include <di/container/iterator/prelude.h>
#include <di/container/linked/linked_list_iterator.h>
#include <di/container/linked/linked_list_node.h>
#include <di/container/meta/prelude.h>
#include <di/math/numeric_limits.h>
#include <di/platform/prelude.h>
#include <di/util/exchange.h>
#include <di/util/reference_wrapper.h>
#include <di/vocab/optional/prelude.h>

namespace di::container {
template<typename T,
         concepts::AllocatorOf<ConcreteLinkedListNode<T>> Alloc = DefaultAllocator<ConcreteLinkedListNode<T>>>
class LinkedList {
private:
    using Node = ConcreteLinkedListNode<T>;
    using Iterator = LinkedListIterator<T>;
    using ConstIterator = meta::ConstIterator<Iterator>;

    template<concepts::InputContainer Con, typename... Args>
    requires(concepts::ContainerCompatible<Con, T>)
    constexpr friend auto tag_invoke(types::Tag<util::create_in_place>, InPlaceType<LinkedList>, Con&& container) {
        auto result = LinkedList {};
        return invoke_as_fallible([&] {
                   return result.append_container(util::forward<Con>(container));
               }) % [&] {
            return util::move(result);
        } | try_infallible;
    }

public:
    constexpr LinkedList() : m_head(&m_head, &m_head) {}

    constexpr LinkedList(LinkedList&& other) : LinkedList() { *this = util::move(other); }

    constexpr LinkedList& operator=(LinkedList&& other) {
        if (this != &other) {
            clear();
            if (!other.empty()) {
                m_head = util::exchange(other.m_head, { &other.m_head, &other.m_head });
                m_size = util::exchange(other.m_size, 0);

                m_head.next->prev = &m_head;
                m_head.prev->next = &m_head;
            }
        }
        return *this;
    }

    constexpr ~LinkedList() { clear(); }

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

    constexpr auto back() {
        return lift_bool(!empty()) % [&] {
            return util::ref(*--end());
        };
    }

    constexpr auto back() const {
        return lift_bool(!empty()) % [&] {
            return util::ref(*--end());
        };
    }

    constexpr Iterator begin() { return Iterator(m_head.next); }
    constexpr Iterator end() { return Iterator(&m_head); }

    constexpr ConstIterator begin() const { return Iterator(m_head.next); }
    constexpr ConstIterator end() const { return Iterator(const_cast<LinkedListNode*>(&m_head)); }

    constexpr bool empty() const { return size() == 0u; }
    constexpr size_t size() const { return m_size; }
    constexpr size_t max_size() const { return math::NumericLimits<size_t>::max; }

    constexpr void clear() { erase(begin(), end()); }

    constexpr Iterator insert(ConstIterator position, T const& value)
    requires(concepts::CopyConstructible<T>)
    {
        return emplace(position, value);
    }
    constexpr Iterator insert(ConstIterator position, T&& value) { return emplace(position, util::move(value)); }

    template<typename... Args>
    requires(concepts::ConstructibleFrom<T, Args...>)
    constexpr decltype(auto) emplace(ConstIterator position, Args&&... args) {
        return as_fallible(create_node(util::forward<Args>(args)...)) % [&](auto* node) {
            auto* next = position.base().node();
            auto* prev = next->prev;

            node->next = next;
            node->prev = prev;

            prev->next = node;
            next->prev = node;
            ++m_size;
            return Iterator(node);
        } | try_infallible;
    }

    template<concepts::ContainerCompatible<T> Con>
    constexpr auto insert_container(ConstIterator position, Con&& container) {
        auto temp = LinkedList {};
        return invoke_as_fallible([&] {
                   return container::sequence(util::forward<Con>(container), [&]<typename X>(X&& value) {
                       return as_fallible(temp.emplace_back(util::forward<X>(value)));
                   });
               }) % [&] {
            this->splice(position, temp);
        } | try_infallible;
    }

    constexpr Iterator erase(ConstIterator position) { return erase(position, container::next(position, 1)); }
    constexpr Iterator erase(ConstIterator start, ConstIterator last) {
        auto* start_node = start.base().node();
        auto* end_node = last.base().node();

        auto* prev_node = start_node->prev;
        prev_node->next = end_node;
        end_node->prev = prev_node;

        while (start != last) {
            auto save = start++;
            destroy_node(save.base().concrete_node());
            --m_size;
        }
        return last.base();
    }

    constexpr decltype(auto) push_back(T const& value)
    requires(concepts::CopyConstructible<T>)
    {
        return emplace_back(value);
    }

    constexpr decltype(auto) push_back(T&& value) { return emplace_back(util::move(value)); }

    template<typename... Args>
    requires(concepts::ConstructibleFrom<T, Args...>)
    constexpr decltype(auto) emplace_back(Args&&... args) {
        return as_fallible(emplace(end(), util::forward<Args>(args)...)) % [](auto it) {
            return util::ref(*it);
        } | try_infallible;
    }

    template<concepts::ContainerCompatible<T> Con>
    constexpr auto append_container(Con&& container) {
        return insert_container(end(), util::forward<Con>(container));
    }

    constexpr Optional<T> pop_back() {
        if (empty()) {
            return nullopt;
        }
        auto last = --end();
        auto value = util::move(*last);
        erase(last);
        return value;
    }

    constexpr decltype(auto) push_front(T const& value)
    requires(concepts::CopyConstructible<T>)
    {
        return emplace_front(value);
    }

    constexpr decltype(auto) push_front(T&& value) { return emplace_front(util::move(value)); }

    template<typename... Args>
    requires(concepts::ConstructibleFrom<T, Args...>)
    constexpr decltype(auto) emplace_front(Args&&... args) {
        return as_fallible(emplace(begin(), util::forward<Args>(args)...)) % [](auto it) {
            return util::ref(*it);
        } | try_infallible;
    }

    template<concepts::ContainerCompatible<T> Con>
    constexpr auto prepend_container(Con&& container) {
        return insert_container(begin(), util::forward<Con>(container));
    }

    constexpr Optional<T> pop_front() {
        if (empty()) {
            return nullopt;
        }
        auto first = begin();
        auto value = util::move(*first);
        erase(first);
        return value;
    }

    constexpr void splice(ConstIterator position, LinkedList& other) { splice(position, util::move(other)); }
    constexpr void splice(ConstIterator position, LinkedList&& other) {
        splice(position, other, other.begin(), other.end());
    }

    constexpr void splice(ConstIterator position, LinkedList& other, ConstIterator it) {
        splice(position, util::move(other), it);
    }
    constexpr void splice(ConstIterator position, LinkedList&& other, ConstIterator it) {
        splice(position, other, it, container::next(it, 1));
    }

    constexpr void splice(ConstIterator position, LinkedList& other, ConstIterator first, ConstIterator last) {
        splice(position, util::move(other), first, last);
    }
    constexpr void splice(ConstIterator position, LinkedList&& other, ConstIterator first, ConstIterator last) {
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
        if (this != &other) {
            auto size = 1 + container::distance(first, ConstIterator(Iterator(inclusive_last_node)));
            this->m_size += size;
            other.m_size -= size;
        }
    }

private:
    constexpr friend bool operator==(LinkedList const& a, LinkedList const& b)
    requires(concepts::EqualityComparable<T>)
    {
        return container::equal(a, b);
    }

    constexpr friend auto operator<=>(LinkedList const& a, LinkedList const& b)
    requires(concepts::ThreeWayComparable<T>)
    {
        return container::compare(a, b);
    }

    template<typename... Args>
    requires(concepts::ConstructibleFrom<T, Args...>)
    constexpr Node* create_node(Args&&... args) {
        auto [pointer, allocated_nodes] = Alloc().allocate(1);
        (void) allocated_nodes;

        util::construct_at(pointer, in_place, util::forward<Args>(args)...);
        return pointer;
    }

    constexpr void destroy_node(Node& node) {
        util::destroy_at(&node);
        Alloc().deallocate(&node, 1);
    }

    LinkedListNode m_head;
    size_t m_size { 0 };
};

template<concepts::InputContainer Con, typename T = meta::ContainerValue<Con>>
LinkedList<T> tag_invoke(types::Tag<util::deduce_create>, InPlaceTemplate<LinkedList>, Con&&);
}