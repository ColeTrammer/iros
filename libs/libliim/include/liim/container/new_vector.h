#pragma once

#include <liim/compare.h>
#include <liim/container/container.h>
#include <liim/format.h>
#include <liim/initializer_list.h>
#include <liim/option.h>
#include <liim/span.h>

namespace LIIM::Container {
template<typename T>
class NewVector;

template<typename VectorType>
class NewVectorIterator {
public:
    using VectorValueType = VectorType::ValueType;
    using ValueType = Conditional<IsConst<VectorType>::value, const VectorValueType&, VectorValueType&>::type;

    constexpr operator NewVectorIterator<const VectorType>() requires(!IsConst<VectorType>::value) {
        return NewVectorIterator<const VectorType>(m_vector, m_index);
    }

    constexpr decltype(auto) operator*() const { return m_vector[m_index]; }
    constexpr decltype(auto) operator->() const { return &m_vector[m_index]; }

    constexpr decltype(auto) operator[](ssize_t index) const { return m_vector[m_index + index]; }

    constexpr NewVectorIterator& operator++() {
        ++m_index;
        return *this;
    }
    constexpr NewVectorIterator& operator--() {
        --m_index;
        return *this;
    }

    constexpr NewVectorIterator operator++(int) { return NewVectorIterator(m_vector, m_index++); }
    constexpr NewVectorIterator operator--(int) { return NewVectorIterator(m_vector, m_index--); }

    constexpr NewVectorIterator operator+(ssize_t n) const { return NewVectorIterator(m_vector, m_index + n); }
    constexpr NewVectorIterator operator-(ssize_t n) const { return NewVectorIterator(m_vector, m_index - n); }

    constexpr ssize_t operator-(const NewVectorIterator& other) const { return this->m_index - other.m_index; }

    constexpr NewVectorIterator& operator+=(ssize_t n) {
        m_index += n;
        return *this;
    }
    constexpr NewVectorIterator& operator-=(ssize_t n) {
        m_index -= n;
        return *this;
    }

    constexpr bool operator==(const NewVectorIterator& other) const { return this->m_index == other.m_index; }
    constexpr bool operator!=(const NewVectorIterator& other) const { return this->m_index != other.m_index; }
    constexpr bool operator<=(const NewVectorIterator& other) const { return this->m_index <= other.m_index; }
    constexpr bool operator<(const NewVectorIterator& other) const { return this->m_index > other.m_index; }
    constexpr bool operator>=(const NewVectorIterator& other) const { return this->m_index >= other.m_index; }
    constexpr bool operator>(const NewVectorIterator& other) const { return this->m_index > other.m_index; }

private:
    explicit constexpr NewVectorIterator(VectorType& vector, size_t index) : m_vector(vector), m_index(index) {}

    template<typename T>
    friend class NewVectorIterator;

    template<typename>
    friend class NewVector;

    VectorType& m_vector;
    size_t m_index { 0 };
};

template<typename T>
class NewVector {
public:
    using ValueType = T;

    constexpr NewVector() = default;
    constexpr NewVector(NewVector&&);

    constexpr static NewVector create(std::initializer_list<T> list);
    template<Iterator Iter>
    constexpr static NewVector create(Iter start, Iter end, Option<size_t> known_size = {});

    constexpr ~NewVector() { clear(); }

    constexpr auto clone() const requires(Cloneable<T>);

    constexpr NewVector& operator=(NewVector&&);

    constexpr NewVector& assign(std::initializer_list<T> list) { return assign(list.begin(), list.end()); }
    template<Iterator Iter>
    constexpr NewVector& assign(Iter start, Iter end, Option<size_t> known_size = {});

    constexpr Option<T&> at(size_t index);
    constexpr Option<const T&> at(size_t index) const;

    T* data() { return reinterpret_cast<T*>(m_data); }
    const T* data() const { return reinterpret_cast<const T*>(m_data); }

    Span<T> span() { return { data(), size() }; }
    Span<const T> span() const { return { data(), size() }; }

    constexpr T& operator[](size_t index) {
        assert(index < size());
        return m_data[index].value;
    }
    constexpr const T& operator[](size_t index) const {
        assert(index < size());
        return m_data[index].value;
    }

    constexpr T& front() { return (*this)[0]; }
    constexpr const T& front() const { return (*this)[0]; }

    constexpr T& back() { return (*this)[size() - 1]; }
    constexpr const T& back() const { return (*this)[size() - 1]; }

    constexpr bool empty() const { return size() == 0; }
    constexpr size_t size() const { return m_size; }
    constexpr size_t capacity() const { return m_capacity; }

    using Iterator = NewVectorIterator<NewVector<T>>;
    using ConstIterator = NewVectorIterator<const NewVector<T>>;

    constexpr auto begin() { return Iterator(*this, 0lu); }
    constexpr auto end() { return Iterator(*this, size()); }
    constexpr auto begin() const { return ConstIterator(*this, 0lu); }
    constexpr auto end() const { return ConstIterator(*this, size()); }
    constexpr auto cbegin() const { return begin(); }
    constexpr auto cend() const { return end(); }
    constexpr auto rbegin() { return ReverseIterator(end()); }
    constexpr auto rend() { return ReverseIterator(Iterator(begin())); }
    constexpr auto rbegin() const { return ReverseIterator(ConstIterator(end())); }
    constexpr auto rend() const { return ReverseIterator(ConstIterator(begin())); }
    constexpr auto crbegin() const { return ReverseIterator(ConstIterator(end())); }
    constexpr auto crend() const { return ReverseIterator(ConstIterator(begin())); }

    constexpr auto iterator(size_t index) { return begin() + index; }
    constexpr auto iterator(size_t index) const { return begin() + index; }
    constexpr auto citerator(size_t index) const { return begin() + index; }

    constexpr void reserve(size_t capacity);

    constexpr void clear();

    constexpr Iterator insert(ConstIterator position, const T& value) { return emplace(position, value); }
    constexpr Iterator insert(ConstIterator position, T&& value) { return emplace(position, move(value)); }
    constexpr Iterator insert(ConstIterator position, std::initializer_list<T> list) { return insert(iterator_index(position), list); }

    template<Container C>
    constexpr Iterator insert(ConstIterator position, C&& container) {
        return insert(iterator_index(position), forward<C>(container));
    }

    template<::Iterator Iter>
    constexpr Iterator insert(ConstIterator position, Iter begin, Iter end, Option<size_t> known_size = {}) {
        return insert(iterator_index(position), move(begin), move(end), known_size);
    }

    constexpr Iterator insert(size_t index, const T& value) { return emplace(index, value); }
    constexpr Iterator insert(size_t index, T&& value) { return emplace(index, move(value)); }
    constexpr Iterator insert(size_t index, std::initializer_list<T> list) { return insert(index, list.begin(), list.end()); }

    template<::Iterator Iter>
    constexpr Iterator insert(size_t index, Iter begin, Iter end, Option<size_t> known_size = {});

    constexpr Iterator erase(ConstIterator position) { return erase(position, position + 1); }
    constexpr Iterator erase(ConstIterator start, ConstIterator end) { return erase_count(iterator_index(start), end - start); }
    constexpr Iterator erase_count(size_t index, size_t count);
    constexpr Iterator erase_unstable(ConstIterator position) { return erase_unstable(iterator_index(position)); }
    constexpr Iterator erase_unstable(size_t index) {
        ::swap((*this)[index], back());
        pop_back();
        return iterator(index);
    }

    template<typename... Args>
    constexpr Iterator emplace(ConstIterator position, Args&&... args) {
        return emplace(iterator_index(position), forward<Args>(args)...);
    }
    template<typename... Args>
    constexpr Iterator emplace(size_t index, Args&&... args);

    constexpr void push_back(const T& value) { emplace_back(value); }
    constexpr void push_back(T&& value) { emplace_back(::move(value)); }

    template<typename... Args>
    constexpr T& emplace_back(Args&&... args) {
        return *emplace(end(), forward<Args>(args)...);
    }

    constexpr Option<T> pop_back();

    constexpr void resize(size_t count, const T& value = T());

    constexpr void swap(NewVector&);

    constexpr bool operator==(const NewVector& other) const requires(EqualComparable<T>);
    constexpr auto operator<=>(const NewVector& other) const requires(Comparable<T>);

private:
    constexpr void move_objects(MaybeUninit<T>* destination, MaybeUninit<T>* source, size_t count);
    constexpr void grow_to(size_t new_size);
    constexpr size_t iterator_index(ConstIterator iterator) const { return iterator - begin(); }

    size_t m_size { 0 };
    size_t m_capacity { 0 };
    MaybeUninit<T>* m_data { nullptr };
};

template<typename T>
constexpr NewVector<T>::NewVector(NewVector<T>&& other)
    : m_size(exchange(other.m_size, 0)), m_capacity(exchange(other.m_capacity, 0)), m_data(exchange(other.m_data, nullptr)) {}

template<typename T>
constexpr auto NewVector<T>::create(std::initializer_list<T> list) -> NewVector {
    auto result = NewVector {};
    result.insert(result.begin(), list);
    return result;
}

template<typename T>
template<Iterator Iter>
constexpr auto NewVector<T>::create(Iter start, Iter end, Option<size_t> known_size) -> NewVector {
    auto result = NewVector {};
    result.insert(result.begin(), move(start), move(end), known_size);
    return result;
}

template<typename T>
constexpr NewVector<T>& NewVector<T>::operator=(NewVector<T>&& other) {
    if (this != &other) {
        NewVector<T> temp(move(other));
        swap(temp);
    }
    return *this;
}

template<typename T>
template<Iterator Iter>
constexpr auto NewVector<T>::assign(Iter start, Iter end, Option<size_t> known_size) -> NewVector& {
    return *this = NewVector::create(move(start), move(end), known_size);
}

template<typename T>
constexpr auto NewVector<T>::clone() const requires(Cloneable<T>) {
    auto result = NewVector<T> {};
    result.reserve(size());
    for (auto& x : *this) {
        result.push_back(::clone(x));
    }
    return result;
}

template<typename T>
constexpr void NewVector<T>::reserve(size_t new_capacity) {
    if (new_capacity <= m_capacity) {
        return;
    }

    auto* old_buffer = m_data;
    auto* new_buffer = new MaybeUninit<T>[new_capacity];
    move_objects(new_buffer, old_buffer, size());
    m_capacity = new_capacity;
    m_data = new_buffer;

    delete[] old_buffer;
}

template<typename T>
constexpr void NewVector<T>::move_objects(MaybeUninit<T>* destination, MaybeUninit<T>* source, size_t count) {
    // NOTE: this should use memmove for trivial types when not in constant evaluated context.
    // Loop backwards, explitly use unsigned underflow in the loop.
    for (size_t i = count - 1; i < count; i--) {
        construct_at(&destination[i].value, move(source[i].value));
        source[i].destroy();
    }
}

template<typename T>
constexpr void NewVector<T>::grow_to(size_t new_size) {
    if (new_size <= m_capacity) {
        return;
    }

    if (m_capacity == 0 || 2 * m_capacity < new_size) {
        reserve(max(20lu, new_size));
    } else {
        reserve(3 * m_capacity / 2);
    }
}

template<typename T>
constexpr Option<T&> NewVector<T>::at(size_t index) {
    if (index >= size()) {
        return None {};
    }
    return Option<T&>((*this)[index]);
}

template<typename T>
constexpr Option<const T&> NewVector<T>::at(size_t index) const {
    if (index >= size()) {
        return None {};
    }
    return Option<const T&>((*this)[index]);
}

template<typename T>
constexpr void NewVector<T>::clear() {
    erase(begin(), end());

    delete[] m_data;
    m_capacity = 0;
    m_data = nullptr;
}

template<typename T>
template<Iterator Iter>
constexpr auto NewVector<T>::insert(size_t index, Iter start, Iter end, Option<size_t>) -> Iterator {
    auto result = index;
    for (auto&& value : iterator_container(move(start), move(end))) {
        insert(index++, static_cast<decltype(value)&&>(value));
    }
    return iterator(result);
}

template<typename T>
template<typename... Args>
constexpr auto NewVector<T>::emplace(size_t index, Args&&... args) -> Iterator {
    grow_to(size() + 1);

    move_objects(m_data + index + 1, m_data + index, size() - index);
    create_at(&m_data[index].value, forward<Args>(args)...);
    m_size++;

    return iterator(index);
}

template<typename T>
constexpr auto NewVector<T>::erase_count(size_t index, size_t count) -> Iterator {
    if (count == 0) {
        return iterator(index);
    }

    auto leftover_elements = size() - index - count;
    for (size_t i = 0; i < leftover_elements; i++) {
        (*this)[index + i] = move((*this)[index + i + count]);
        m_data[index + i + count].destroy();
    }
    for (size_t i = index + leftover_elements; i < m_size; i++) {
        m_data[i].destroy();
    }
    m_size -= count;
    return iterator(min(index, size()));
}

template<typename T>
constexpr Option<T> NewVector<T>::pop_back() {
    if (empty()) {
        return None {};
    }

    auto& slot = m_data[--m_size];
    auto value = Option<T>(move(slot.value));
    slot.destroy();
    return value;
}

template<typename T>
constexpr void NewVector<T>::resize(size_t n, const T& value) {
    if (size() > n) {
        erase_count(n, size() - n);
    } else {
        ::insert(*this, end(), repeat(n - size(), value));
    }
}

template<typename T>
constexpr bool NewVector<T>::operator==(const NewVector& other) const requires(EqualComparable<T>) {
    if (this->size() != other.size()) {
        return false;
    }
    for (auto [left, right] : zip(*this, other)) {
        if (left != right) {
            return false;
        }
    }
    return true;
}

template<typename T>
constexpr auto NewVector<T>::operator<=>(const NewVector& other) const requires(Comparable<T>) {
    for (auto [left, right] : zip(*this, other)) {
        if (auto result = left <=> right; result != 0) {
            return result;
        }
    }
    return static_cast<ThreeWayCompareResult<T>>(this->size() <=> other.size());
}

template<typename T>
constexpr void NewVector<T>::swap(NewVector<T>& other) {
    ::swap(this->m_size, other.m_size);
    ::swap(this->m_capacity, other.m_capacity);
    ::swap(this->m_data, other.m_data);
}

template<typename T>
constexpr auto make_vector(std::initializer_list<T> list) {
    return NewVector<T>::create(list);
}

template<Iterator Iter>
constexpr auto make_vector(Iter start, Iter end, Option<size_t> known_size = {}) {
    using Type = decay_t<typename IteratorTraits<Iter>::ValueType>;
    return NewVector<Type>::create(move(start), move(end), known_size);
}

template<Container C>
constexpr auto collect_vector(C&& container) {
    using Iter = decltype(container.begin());
    using ValueType = IteratorTraits<Iter>::ValueType;
    return collect<NewVector<decay_t<ValueType>>>(forward<C>(container));
}

template<typename T>
constexpr void swap(NewVector<T>& a, NewVector<T>& b) {
    a.swap(b);
}
}

namespace LIIM::Format {
template<Formattable T>
struct Formatter<LIIM::Container::NewVector<T>> {
    constexpr void parse(FormatParseContext& context) { m_formatter.parse(context); }

    void format(const LIIM::Container::NewVector<T>& vector, FormatContext& context) {
        context.put("[ ");
        bool first = true;
        for (auto& item : vector) {
            if (!first) {
                context.put(", ");
            }
            m_formatter.format(item, context);
            first = false;
        }
        context.put(" ]");
    }

    Formatter<T> m_formatter;
};
}

using LIIM::Container::collect_vector;
using LIIM::Container::make_vector;
using LIIM::Container::NewVector;
