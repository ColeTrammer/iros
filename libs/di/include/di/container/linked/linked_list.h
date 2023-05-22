#pragma once

#include <di/container/algorithm/compare.h>
#include <di/container/algorithm/equal.h>
#include <di/container/allocator/allocate_one.h>
#include <di/container/allocator/allocator.h>
#include <di/container/allocator/deallocate_one.h>
#include <di/container/allocator/fallible_allocator.h>
#include <di/container/allocator/infallible_allocator.h>
#include <di/container/concepts/prelude.h>
#include <di/container/intrusive/prelude.h>
#include <di/container/iterator/prelude.h>
#include <di/container/meta/prelude.h>
#include <di/math/numeric_limits.h>
#include <di/platform/prelude.h>
#include <di/util/exchange.h>
#include <di/util/reference_wrapper.h>
#include <di/vocab/expected/prelude.h>
#include <di/vocab/optional/prelude.h>

namespace di::container {
namespace detail {
    template<typename T>
    struct LinkedListTag;

    template<typename T>
    struct LinkedListNode : IntrusiveListNode<LinkedListTag<T>> {
    public:
        template<typename... Args>
        requires(concepts::ConstructibleFrom<T, Args...>)
        constexpr LinkedListNode(InPlace, Args&&... args) : m_value(util::forward<Args>(args)...) {}

        constexpr T& value() { return m_value; }

    private:
        T m_value;
    };

    template<typename T>
    struct LinkedListTag : IntrusiveTagBase<LinkedListNode<T>> {
        using Node = LinkedListNode<T>;

        constexpr static bool is_sized(InPlaceType<T>) { return true; }
        constexpr static T& down_cast(InPlaceType<T>, Node& node) { return node.value(); }

        constexpr static void did_remove(auto& list, auto& node) {
            util::destroy_at(util::addressof(node));
            di::deallocate_one<Node>(list.allocator(), util::addressof(node));
        }
    };
}

template<typename T, concepts::Allocator Alloc = DefaultAllocator>
class LinkedList : public IntrusiveList<T, detail::LinkedListTag<T>, LinkedList<T, Alloc>> {
private:
    using Node = detail::LinkedListNode<T>;
    using List = IntrusiveList<T, detail::LinkedListTag<T>, LinkedList<T, Alloc>>;
    using Iterator = meta::ContainerIterator<List>;
    using ConstIterator = meta::ContainerConstIterator<List>;

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
    LinkedList() = default;

    LinkedList(LinkedList&&) = default;
    LinkedList& operator=(LinkedList&&) = default;

    ~LinkedList() = default;

    constexpr Iterator insert(ConstIterator position, T const& value)
    requires(concepts::CopyConstructible<T>)
    {
        return emplace(position, value);
    }
    constexpr Iterator insert(ConstIterator position, T&& value) { return emplace(position, util::move(value)); }

    template<typename... Args>
    requires(concepts::ConstructibleFrom<T, Args...>)
    constexpr decltype(auto) emplace(ConstIterator position, Args&&... args) {
        return as_fallible(create_node(util::forward<Args>(args)...)) % [&](Node& node) {
            return List::insert(position, node);
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

    constexpr decltype(auto) push_back(T const& value)
    requires(concepts::CopyConstructible<T>)
    {
        return emplace_back(value);
    }

    constexpr decltype(auto) push_back(T&& value) { return emplace_back(util::move(value)); }

    template<typename... Args>
    requires(concepts::ConstructibleFrom<T, Args...>)
    constexpr decltype(auto) emplace_back(Args&&... args) {
        return as_fallible(emplace(this->end(), util::forward<Args>(args)...)) % [](Iterator it) {
            return util::ref(*it);
        } | try_infallible;
    }

    template<concepts::ContainerCompatible<T> Con>
    constexpr auto append_container(Con&& container) {
        return insert_container(this->end(), util::forward<Con>(container));
    }

    constexpr Optional<T> pop_back() {
        if (this->empty()) {
            return nullopt;
        }
        auto last = --this->end();
        auto value = util::move(*last);
        this->erase(last);
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
        return as_fallible(emplace(this->begin(), util::forward<Args>(args)...)) % [](auto it) {
            return util::ref(*it);
        } | try_infallible;
    }

    template<concepts::ContainerCompatible<T> Con>
    constexpr auto prepend_container(Con&& container) {
        return insert_container(this->begin(), util::forward<Con>(container));
    }

    constexpr Optional<T> pop_front() {
        if (this->empty()) {
            return nullopt;
        }
        auto first = this->begin();
        auto value = util::move(*first);
        this->erase(first);
        return value;
    }

    constexpr Alloc& allocator() { return m_allocator; }

private:
    template<typename... Args>
    requires(concepts::ConstructibleFrom<T, Args...>)
    constexpr decltype(auto) create_node(Args&&... args) {
        return as_fallible(di::allocate_one<Node>(m_allocator)) % [&](Node* pointer) {
            util::construct_at(pointer, in_place, util::forward<Args>(args)...);
            return util::ref(*static_cast<Node*>(pointer));
        } | try_infallible;
    }

    [[no_unique_address]] Alloc m_allocator {};
};

template<concepts::InputContainer Con, typename T = meta::ContainerValue<Con>>
LinkedList<T> tag_invoke(types::Tag<util::deduce_create>, InPlaceTemplate<LinkedList>, Con&&);
}
