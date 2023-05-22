# Allocator

## Purpose

Controlling the mechanism for allocating memory is very useful for performance sensitive applications. In particular,
many functions require allocating memory, and must also consider the possibility of allocation failure. Additionally,
since coroutines allocate memory in c++, a good allocator mechanism is needed.

## Problem with Standard C++ Allocators

The original model for the library is based directly on standard c++ allocators. However, there are a few problems with
this approach.

1. Allocators are templated on a concrete type, which makes their use cumbersome.
2. As a direct result of (1), allocators cannot be type-erased.

## Allocator Interface

The allocator interface provides a way to allocate memory, and to deallocate memory.

```cpp
template<typename T = void>
struct AllocationResult {
    T* data;
    usize count;
};

template <typename T>
concept Allocator = requires(T& allocator, void* ptr, usize size, usize alignment) {
    { di::allocate(allocator, size, alignment) } -> MaybeFallible<AllocationResult<>>;
    di::deallocate(allocator, ptr, size, alignment);
};
```

The `allocate` function returns an `AllocationResult`, which contains the pointer to the allocated memory, and the
actual size in bytes of the allocation. This is useful for allocators which may allocate more memory than requested, and
can be used by `di::Vector` to determine the capacity of the vector. In addition, this operation is allowed to fail for
any reason, but can optionally be infallible if the allocator asserts on failure.

The `deallocate` function is the inverse of `allocate`, and deallocates the memory at the given pointer. The size and
alignment must be provided, which allows much more efficent implementations of `deallocate`.

Both functions are implemented as CPOs, which means that they can be defined in terms of member functions `allocate` and
`deallocate` or using hidden-friend `tag_invoke` overloads, which means that they can be type-erased using `di::Any`.

Note that this concept specifies no constraints on the allocator's copy or move semantics. However, allocators which are
used in containers have to consider these semantics. It is expected that a container will inherit the movability of its
allocator, and will only be cloneable if its allocator is cloneable.

## Allocator Usage

These CPOs can be used directly, but more commonly, code wants to allocate or deallocate a specific type. This is done
using the `di::allocate_many` and `di::allocate_one` functions, which have the following signatures:

```cpp
template<typename T>
constexpr MaybeFallible<T*> allocate_one(Allocator auto& allocator);

template<typename T>
constexpr MaybeFallible<AllocationResult<T>> allocate_many(Allocator auto& allocator, usize count);

template<typename T>
constexpr void deallocate_one(Allocator auto& allocator, T* ptr);

template<typename T>
constexpr void deallocate_many(Allocator auto& allocator, T* ptr, usize count);
```

These functions call to the underlying byte-allocation strategy, and then cast the pointer to the appropriate type. When
called in a constant-evaulated context, these functions will use `std::allocator<T>` instead of the passed allocator,
since this is the only way to support constexpr allocation.

## Allocator Interaction with Containers

All owning containers in the library have an allocator type parameter. The biggest question when using allocators is
what to do when moving out of a container. The library has two options for this:

1. The container can move the values into memory allocated by the new container's allocator.
2. The container can move the simply move the old allocator into the new container.

The first option is what is chosen by the standard polymorphic allocators, but is not what is chosen by the library.
This is probably because option 2 requires type-erasing a move operation, and additionally makes inline storage in a
memory resource impossible. Furthermore, standard allocators are meant to have "reference" semantics. However, since
this library supports type-erasure already, as well as static vectors, option 2 is chosen. This results in a lot more
intuitive behavior, and prevents needless allocation. On the other hand, this introduces safety issues, since the
container's allocator may need to be re-initialized after being moved out of. As a consequence, allocators which have
"destuctive" moves should be fallible, and should return an error if an allocation occurs in this state.

## Conceptual Example

```cpp
auto pool = di::make_pool_allocator(8192);
auto vector = di::Vector<int, di::AnyInline<di::Allocator>>(di::move(pool));

TRY(vector.push_back(1));
TRY(vector.push_back(2));

auto new_vector = di::move(vector);

// This is using the same allocator as the old vector.
TRY(new_vector.push_back(3));
ASSERT_EQ(new_vector.size(), 3);

// Allocation to the pool allocator is not allowed after moving out of it.
ASSERT(!vector.push_back(2));
```
