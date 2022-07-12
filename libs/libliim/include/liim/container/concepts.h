#pragma once

#include <liim/compare.h>
#include <liim/construct.h>
#include <liim/utilities.h>

namespace LIIM::Container {
template<typename T>
struct IteratorTraits {
    using ValueType = T::ValueType;
};

template<typename T>
struct IteratorTraits<T*> {
    using ValueType = T&;
};

template<typename T>
concept Iterator = EqualComparable<T> && requires(T iterator) {
    { *iterator } -> SameAs<typename IteratorTraits<T>::ValueType>;
    { ++iterator } -> SameAs<T&>;
};

template<typename T>
concept DoubleEndedIterator = Iterator<T> && requires(T iterator) {
    { --iterator } -> SameAs<T&>;
};

template<typename T>
concept RandomAccessIterator = DoubleEndedIterator<T> && Copyable<T> && Comparable<T> && requires(T iterator, T other, size_t index) {
    { iterator[index] } -> SameAs<typename IteratorTraits<T>::ValueType>;
    { iterator - other } -> SameAs<ssize_t>;
};

template<typename T>
concept Container = requires(T container) {
    { container.begin() } -> Iterator;
    { container.end() } -> Iterator;
};

template<typename T>
concept DoubleEndedContainer = requires(T container) {
    { container.begin() } -> DoubleEndedIterator;
    { container.end() } -> DoubleEndedIterator;
};

template<typename T>
concept RandomAccessContainer = requires(T container) {
    { container.begin() } -> RandomAccessIterator;
    { container.end() } -> RandomAccessIterator;
};

template<typename T>
concept SizedContainer = Container<T> && requires(T container) {
    { container.size() } -> SameAs<size_t>;
};

template<typename T>
concept MemberContentSwappableIterator = Iterator<T> && Copyable<T> && requires(T a, T b) {
    a.swap_contents(b);
};

template<Container C>
using IteratorForContainer = decltype(declval<C>().end());

template<Container C>
using ConstIteratorForContainer = decltype(declval<const C>().end());

template<Iterator Iter>
using IteratorValueType = IteratorTraits<Iter>::ValueType;

template<Container C>
using ContainerValueType = IteratorValueType<IteratorForContainer<C>>;

template<typename T>
concept MutableIterator = Iterator<T> &&(MemberContentSwappableIterator<T> || IsLValueReference<IteratorValueType<T>>::value);

template<typename T>
concept MutableContainer = Container<T> && requires(T container) {
    { container.begin() } -> MutableIterator;
    { container.end() } -> MutableIterator;
};

template<typename T>
concept MutableDoubleEndedIterator = MutableIterator<T> && DoubleEndedIterator<T>;

template<typename T>
concept MutableDoubleEndedContainer = MutableContainer<T> && DoubleEndedContainer<T>;

template<typename T>
concept MutableRandomAccessIterator = MutableIterator<T> && RandomAccessIterator<T>;

template<typename T>
concept MutableRandomAccessContainer = MutableContainer<T> && RandomAccessContainer<T>;

template<typename T>
concept Clearable = Container<T> && requires(T& container) {
    container.clear();
};

template<typename C, typename T>
concept InsertableFor = Container<C> && requires(C& container, IteratorForContainer<C> iter) {
    { container.insert(move(iter), declval<T>()) } -> SameAs<IteratorForContainer<C>>;
};

template<typename C, typename T>
concept FalliblyInsertableFor = Container<C> && requires(C& container, IteratorForContainer<C> iter) {
    { container.insert(move(iter), declval<T>()) } -> ResultOf<IteratorForContainer<C>>;
};
}

using LIIM::Container::Clearable;
using LIIM::Container::ConstIteratorForContainer;
using LIIM::Container::Container;
using LIIM::Container::ContainerValueType;
using LIIM::Container::DoubleEndedContainer;
using LIIM::Container::DoubleEndedIterator;
using LIIM::Container::Iterator;
using LIIM::Container::IteratorForContainer;
using LIIM::Container::IteratorValueType;
using LIIM::Container::MutableContainer;
using LIIM::Container::MutableDoubleEndedContainer;
using LIIM::Container::MutableDoubleEndedIterator;
using LIIM::Container::MutableIterator;
using LIIM::Container::MutableRandomAccessContainer;
using LIIM::Container::MutableRandomAccessIterator;
using LIIM::Container::RandomAccessContainer;
using LIIM::Container::RandomAccessIterator;
