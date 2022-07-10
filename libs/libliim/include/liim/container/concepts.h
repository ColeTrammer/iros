#pragma once

#include <liim/compare.h>
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
concept RandomAccessIterator = DoubleEndedIterator<T> && Comparable<T> && requires(T iterator, size_t index) {
    { iterator[index] } -> SameAs<typename IteratorTraits<T>::ValueType>;
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

template<Container C>
using IteratorForContainer = decltype(declval<C>().end());

template<Container C>
using ConstIteratorForContainer = decltype(declval<const C>().end());

template<Iterator Iter>
using IteratorValueType = IteratorTraits<Iter>::ValueType;

template<Container C>
using ContainerValueType = IteratorValueType<IteratorForContainer<C>>;

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
using LIIM::Container::RandomAccessContainer;
using LIIM::Container::RandomAccessIterator;
