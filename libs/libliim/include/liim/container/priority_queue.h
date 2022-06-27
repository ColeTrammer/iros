#pragma once

#include <liim/compare.h>
#include <liim/container/new_vector.h>

namespace LIIM::Container {
template<typename T, ComparatorFor<T> Comp = Less<T>>
class PriorityQueue : public ValueIteratorAdapter<PriorityQueue<T, Comp>> {
public:
    using ValueType = T;
    using Iterator = ValueIteratorAdapter<PriorityQueue<T, Comp>>::Iterator;
    using ConstIterator = ValueIteratorAdapter<PriorityQueue<T, Comp>>::ConstIterator;

    constexpr static auto create(std::initializer_list<T> list, Comp&& comparator = Comp()) {
        return create(list.begin(), list.end(), {}, forward<Comp>(comparator));
    }
    template<::Iterator Iter>
    constexpr static auto create(Iter start, Iter end, Option<size_t> known_size = {}, Comp&& compator = Comp());

    constexpr PriorityQueue() = default;
    constexpr PriorityQueue(Comp&& comparator) : m_comparator(forward<Comp>(comparator)) {}
    constexpr PriorityQueue(PriorityQueue&&) = default;

    constexpr PriorityQueue& operator=(PriorityQueue&&) = default;

    constexpr auto clone() const requires(Cloneable<Comp>);

    constexpr bool empty() const { return size() == 0; }
    constexpr size_t size() const { return m_data.size(); }
    constexpr size_t capacity() const { return m_data.capacity(); }

    void clear() { m_data.clear(); }

    constexpr Option<T> next() { return maybe_pop(); }

    constexpr T& top() { return m_data[0]; }
    constexpr const T& top() const { return m_data[0]; }

    constexpr auto push(const T& value) requires(Copyable<T>) { return insert(this->end(), value); }
    constexpr auto push(T&& value) { return insert(this->end(), move(value)); }

    template<typename... Args>
    requires(CreateableFrom<T, Args...> || FalliblyCreateableFrom<T, Args...>) constexpr auto emplace(Args&&... args) {
        return insert(this->end(), forward<Args>(args)...);
    }

    template<typename... Args>
    requires(CreateableFrom<T, Args...> || FalliblyCreateableFrom<T, Args...>) constexpr auto insert(ConstIterator hint, Args&&... args);

    template<::Iterator Iter>
    constexpr Iterator insert(ConstIterator hint, Iter start, Iter end, Option<size_t> known_size = {});

    constexpr T pop();
    constexpr Option<T> maybe_pop();

private:
    constexpr void bubble_up(size_t index);
    constexpr size_t parent_index(size_t index) const;
    constexpr Pair<Option<size_t>, Option<size_t>> child_indices(size_t index) const;
    constexpr bool greater_than(const T& a, const T& b) const;

    NewVector<T> m_data;
    Comp m_comparator;
};

template<typename T, ComparatorFor<T> Comp>
template<Iterator Iter>
constexpr auto PriorityQueue<T, Comp>::create(Iter start, Iter end, Option<size_t> known_size, Comp&& comp) {
    auto result = PriorityQueue(forward<Comp>(comp));
    return result_and_then(result.insert(result.end(), move(start), move(end), known_size), [&](auto&&) -> PriorityQueue {
        return move(result);
    });
}

template<typename T, ComparatorFor<T> Comp>
constexpr auto PriorityQueue<T, Comp>::clone() const requires(Cloneable<Comp>) {
    return result_and_then(::clone(m_comparator), [&](auto&& comparator) {
        auto result = PriorityQueue { move(comparator) };
        return result_and_then(m_data.clone(), [&](auto&& data) -> PriorityQueue {
            result.m_data = move(data);
            return move(result);
        });
    });
}

template<typename T, ComparatorFor<T> Comp>
template<typename... Args>
requires(CreateableFrom<T, Args...> || FalliblyCreateableFrom<T, Args...>) constexpr auto PriorityQueue<T, Comp>::insert(ConstIterator,
                                                                                                                         Args&&... args) {
    return result_and_then(m_data.emplace_back(forward<Args>(args)...), [&](auto&&) {
        bubble_up(size() - 1);
        return this->end();
    });
}

template<typename T, ComparatorFor<T> Comp>
template<Iterator Iter>
constexpr auto PriorityQueue<T, Comp>::insert(ConstIterator, Iter start, Iter end, Option<size_t> known_size) -> Iterator {
    auto old_size = this->size();
    return result_and_then(m_data.insert(m_data.end(), move(start), move(end), known_size), [&](auto&&) {
        for (auto i : range(old_size, size())) {
            bubble_up(i);
        }
        return this->end();
    });
}

template<typename T, ComparatorFor<T> Comp>
constexpr T PriorityQueue<T, Comp>::pop() {
    assert(!empty());
    auto result = T(move(top()));
    m_data.erase_unstable(0);

    size_t index = 0;
    for (;;) {
        auto [left_child, right_child] = child_indices(index);
        if (!left_child) {
            break;
        }
        if (!right_child) {
            if (greater_than(m_data[*left_child], m_data[index])) {
                ::swap(m_data[*left_child], m_data[index]);
            }
            break;
        }

        size_t smallest_child = greater_than(m_data[*left_child], m_data[*right_child]) ? *left_child : *right_child;
        if (greater_than(m_data[index], m_data[smallest_child])) {
            break;
        }
        ::swap(m_data[index], m_data[smallest_child]);
        index = smallest_child;
    }

    return result;
}

template<typename T, ComparatorFor<T> Comp>
constexpr Option<T> PriorityQueue<T, Comp>::maybe_pop() {
    if (empty()) {
        return None {};
    }
    return pop();
}

template<typename T, ComparatorFor<T> Comp>
constexpr void PriorityQueue<T, Comp>::bubble_up(size_t index) {
    for (; index && greater_than(m_data[index], m_data[parent_index(index)]); index = parent_index(index)) {
        ::swap(m_data[index], m_data[parent_index(index)]);
    }
}

template<typename T, ComparatorFor<T> Comp>
constexpr size_t PriorityQueue<T, Comp>::parent_index(size_t index) const {
    if (index == 0) {
        return 0;
    }
    return (index + 1) / 2 - 1;
}

template<typename T, ComparatorFor<T> Comp>
constexpr Pair<Option<size_t>, Option<size_t>> PriorityQueue<T, Comp>::child_indices(size_t index) const {
    auto left_index = 2 * (index + 1) - 1;
    auto right_index = 2 * (index + 1);

    auto maybe_index = [&](size_t index) -> Option<size_t> {
        if (index >= m_data.size()) {
            return None {};
        }
        return index;
    };
    return { maybe_index(left_index), maybe_index(right_index) };
}

template<typename T, ComparatorFor<T> Comp>
constexpr bool PriorityQueue<T, Comp>::greater_than(const T& a, const T& b) const {
    return !m_comparator(a, b);
}

template<typename T, ComparatorFor<T> Comp = Less<T>>
constexpr auto make_priority_queue(std::initializer_list<T> list, Comp&& comp = Comp()) {
    return PriorityQueue<T, Comp>::create(list, forward<Comp>(comp));
}

template<Iterator Iter, typename Comp = Less<decay_t<IteratorValueType<Iter>>>>
constexpr auto make_priority_queue(Iter start, Iter end, Option<size_t> known_size = {}, Comp&& comparator = Comp()) {
    using ValueType = IteratorValueType<Iter>;
    using QueueType = PriorityQueue<decay_t<ValueType>, Comp>;
    return QueueType::create(move(start), move(end), known_size, forward<Comp>(comparator));
}

template<Container C>
constexpr auto collect_priority_queue(C&& container) {
    return collect<PriorityQueue<decay_t<ContainerValueType<C>>>>(forward<C>(container));
}

template<Container C, typename Comp>
constexpr auto collect_priority_queue(C&& container, Comp&& comparator) {
    auto result = PriorityQueue<decay_t<ContainerValueType<C>>, Comp>(forward<Comp>(comparator));
    return result_and_then(insert(result, result.end(), forward<C>(container)), [&](auto&&) {
        return move(result);
    });
}
}

using LIIM::Container::collect_priority_queue;
using LIIM::Container::make_priority_queue;
using LIIM::Container::PriorityQueue;
