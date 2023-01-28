#pragma once

#include <liim/container/forward.h>
#include <liim/error/forward.h>
#include <liim/format/forward.h>
#include <liim/traits.h>

namespace LIIM {
typedef unsigned long size_t;

template<typename T>
concept Hashable = requires(T a, T b) {
    Traits<T>::hash(a);
    a == b;
    a != b;
};

template<typename T>
class Bitset;

class ByteBuffer;
class ByteReader;
class ByteWriter;

template<typename T, size_t max_elements>
class FixedArray;

template<typename>
class Function;
template<typename R, typename... Args>
class Function<R(Args...)>;

template<typename T>
class Generator;

template<Hashable K, typename V>
class HashMap;

template<Hashable T>
class HashSet;

template<typename T>
class InlineLinkedList;

template<typename T>
class InlineQueue;

template<typename T>
class LinkedList;

struct Monostate;

template<typename T>
class Option;

template<typename T, typename E>
class Result;

template<typename T, size_t N>
class RingBuffer;

template<typename T>
class SharedPtr;

template<typename T>
class Span;

template<typename T>
class Stack;

class StringView;
class String;

template<typename T>
class Task;

template<typename... Types>
class Tuple;

template<typename T>
class UniquePtr;

class Utf8View;

template<typename... Types>
class Variant;

template<typename T>
class Vector;

template<typename T>
class WeakPtr;

struct None;

template<typename T>
struct Err;
}

namespace std {
template<typename T>
struct tuple_size;

template<LIIM::size_t index, typename T>
struct tuple_element;
}

using LIIM::Bitset;
using LIIM::ByteBuffer;
using LIIM::ByteReader;
using LIIM::ByteWriter;
using LIIM::Err;
using LIIM::Function;
using LIIM::Generator;
using LIIM::HashMap;
using LIIM::HashSet;
using LIIM::InlineLinkedList;
using LIIM::InlineQueue;
using LIIM::LinkedList;
using LIIM::Monostate;
using LIIM::None;
using LIIM::Option;
using LIIM::Result;
using LIIM::RingBuffer;
using LIIM::SharedPtr;
using LIIM::Span;
using LIIM::Stack;
using LIIM::String;
using LIIM::StringView;
using LIIM::Task;
using LIIM::Tuple;
using LIIM::UniquePtr;
using LIIM::Utf8View;
using LIIM::Variant;
using LIIM::Vector;
using LIIM::WeakPtr;
