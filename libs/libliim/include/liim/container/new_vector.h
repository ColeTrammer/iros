#pragma once

#include <liim/compare.h>
#include <liim/container/container.h>
#include <liim/error/common_result.h>
#include <liim/error/error.h>
#include <liim/error/system_domain.h>
#include <liim/format.h>
#include <liim/initializer_list.h>
#include <liim/option.h>
#include <liim/result.h>
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
    constexpr auto operator<=>(const NewVectorIterator& other) const { return this->m_index <=> other.m_index; }

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
    using ReserveResult = void;

    using Iterator = NewVectorIterator<NewVector<T>>;
    using ConstIterator = NewVectorIterator<const NewVector<T>>;

    template<typename V, typename... Args>
    using InsertResult = CommonResult<V, CreateAtResultDefault<T, Args...>>;

    template<typename V, ::Iterator Iter>
    using IteratorInsertResult = InsertResult<V, IteratorValueType<Iter>>;

    constexpr NewVector() = default;
    constexpr NewVector(NewVector&&);

    constexpr static InsertResult<NewVector, const T&> create(std::initializer_list<T> list) requires(Copyable<T>) {
        auto result = NewVector {};
        result.insert(result.begin(), list);
        return result;
    }

    template<::Iterator Iter>
    constexpr static IteratorInsertResult<NewVector, Iter> create(Iter start, Iter end, Option<size_t> known_size = {});

    constexpr ~NewVector() { clear(); }

    constexpr auto clone() const requires(Cloneable<T> || FalliblyCloneable<T>) {
        return collect<NewVector<T>>(transform(*this, [](const auto& v) {
            return ::clone(v);
        }));
    }

    constexpr NewVector& operator=(NewVector&&);

    constexpr decltype(auto) assign(std::initializer_list<T> list) requires(Copyable<T>) { return assign(list.begin(), list.end()); }

    template<::Iterator Iter>
    constexpr IteratorInsertResult<NewVector&, Iter> assign(Iter start, Iter end, Option<size_t> known_size = {});

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

    constexpr size_t iterator_index(ConstIterator iterator) const { return iterator - begin(); }

    constexpr ReserveResult reserve(size_t capacity);

    constexpr void clear();

    template<typename U>
    constexpr auto insert(ConstIterator position, U&& value) {
        return emplace(position, forward<U>(value));
    }

    constexpr auto insert(ConstIterator position, std::initializer_list<T> list) requires(Copyable<T>) {
        return insert(iterator_index(position), list);
    }

    template<::Iterator Iter>
    constexpr auto insert(ConstIterator position, Iter begin, Iter end, Option<size_t> known_size = {}) {
        return insert(iterator_index(position), move(begin), move(end), known_size);
    }

    template<typename U>
    constexpr auto insert(size_t index, U&& value) {
        return emplace(index, forward<U>(value));
    }

    constexpr auto insert(size_t index, std::initializer_list<T> list) requires(Copyable<T>) {
        return insert(index, list.begin(), list.end());
    }

    template<::Iterator Iter>
    constexpr IteratorInsertResult<Iterator, Iter> insert(size_t index, Iter begin, Iter end, Option<size_t> known_size = {});

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
    constexpr auto emplace(ConstIterator position, Args&&... args) {
        return emplace(iterator_index(position), forward<Args>(args)...);
    }
    template<typename... Args>
    constexpr InsertResult<Iterator, Args...> emplace(size_t index, Args&&... args);

    constexpr decltype(auto) push_back(const T& value) requires(Copyable<T>) { return emplace_back(value); }
    constexpr decltype(auto) push_back(T&& value) { return emplace_back(move(value)); }

    template<typename... Args>
    constexpr InsertResult<T&, Args...> emplace_back(Args&&... args) {
        if constexpr (CreateableFrom<T, Args...>) {
            return *emplace(end(), forward<Args>(args)...);
        } else {
            return result_and_then(emplace(end(), forward<Args>(args)...), [](auto it) -> T& {
                return *it;
            });
        }
    }

    constexpr Option<T> pop_back();

    constexpr InsertResult<void, const T&> resize(size_t count, const T& value = T()) requires(Copyable<T>);

    constexpr void swap(NewVector&);

    constexpr bool operator==(const NewVector& other) const requires(EqualComparable<T>);
    constexpr auto operator<=>(const NewVector& other) const requires(Comparable<T>);

private:
    constexpr void move_objects(MaybeUninit<T>* destination, MaybeUninit<T>* source, size_t count);
    constexpr ReserveResult grow_to(size_t new_size);

    size_t m_size { 0 };
    size_t m_capacity { 0 };
    MaybeUninit<T>* m_data { nullptr };
};

template<typename T>
constexpr NewVector<T>::NewVector(NewVector<T>&& other)
    : m_size(exchange(other.m_size, 0)), m_capacity(exchange(other.m_capacity, 0)), m_data(exchange(other.m_data, nullptr)) {}

template<typename T>
template<Iterator Iter>
constexpr auto NewVector<T>::create(Iter start, Iter end, Option<size_t> known_size) -> IteratorInsertResult<NewVector, Iter> {
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
constexpr auto NewVector<T>::assign(Iter start, Iter end, Option<size_t> known_size) -> IteratorInsertResult<NewVector&, Iter> {
    clear();
    return result_and_then(insert(this->end(), move(start), move(end), known_size), [this](auto) -> NewVector& {
        return *this;
    });
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
        return reserve(max(20lu, new_size));
    } else {
        return reserve(3 * m_capacity / 2);
    }
}

template<typename T>
constexpr Option<T&> NewVector<T>::at(size_t index) {
    if (index >= size()) {
        return None {};
    }
    return (*this)[index];
}

template<typename T>
constexpr Option<const T&> NewVector<T>::at(size_t index) const {
    if (index >= size()) {
        return None {};
    }
    return (*this)[index];
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
constexpr auto NewVector<T>::insert(size_t index, Iter start, Iter end, Option<size_t>) -> IteratorInsertResult<Iterator, Iter> {
    auto result = index;
    if constexpr (CreateableFrom<T, IteratorValueType<Iter>>) {
        for (auto&& value : iterator_container(move(start), move(end))) {
            emplace(index++, static_cast<decltype(value)&&>(value));
        }
    } else if constexpr (FalliblyCreateableFrom<T, IteratorValueType<Iter>>) {
        auto old_end = this->end();
        for (auto it = move(start); it != end; ++it) {
            auto outcome = emplace_back(*it);
            if (!outcome) {
                erase(old_end, this->end());
                return move(outcome).try_did_fail();
            }
        }
        rotate(iterator_container(iterator(index), this->end()), old_end);
    }
    return iterator(result);
}

template<typename T>
template<typename... Args>
constexpr auto NewVector<T>::emplace(size_t index, Args&&... args) -> InsertResult<Iterator, Args...> {
    if constexpr (CreateableFrom<T, Args...>) {
        grow_to(size() + 1);

        move_objects(m_data + index + 1, m_data + index, size() - index);
        create_at(&m_data[index].value, forward<Args>(args)...);

        m_size++;
        return iterator(index);
    } else {
        return result_and_then(LIIM::create<T>(forward<Args>(args)...), [&](T&& value) {
            return emplace(index, move(value));
        });
    }
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
constexpr auto NewVector<T>::resize(size_t n, const T& value) -> InsertResult<void, const T&>
requires(Copyable<T>) {
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

template<Copyable T>
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
