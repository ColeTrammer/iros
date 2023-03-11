# Intrusive Containers

## Comparison with Owning Containers

Traditionally, containers "own" the elements which they reference. For instance, `di::Vector<T>` internally allocates
memory to store each element in the vector. As a consequence, there it is inconvient to store the same value of type `T`
in multiple containers.

Intrusive containers allow the container itself to use the space inside of an element `T` as part of the storage. This
scheme allows data structures like linked-lists and trees to be efficently implemented without allocation. Storing nodes
on the stack is useful in cases where the a thread needs to store add itself to a linked list, and then block. Also, in
multi-threaded applications, an intrusive container protected by a lock can be used without needing to access the global
allocator. This is criticial in kernelspace, because allocation is fallible and requires synchronization. By using
intrusive containers, kernel code doesn't need to handle insertion failures, and doesn't hold locks to containers while
performing the actual allocation.

## Main Concern with Intrusive Containers

The main issue with intrusive containers is that they do not have owner-ship semantics. In particular, there is no way
to enforce that the nodes being referenced are valid. Sometimes this is by design, when allocating on the stack before
blocking. In other cases, code must make use of reference-counting to ensure object validity.

Another important point is that, from an implementation perspective, it would be nice to implement owning containers in
terms of intrusive containers. This ensures that the 2 containers will have feature parity. But to do so, we need to
introduce some customization points for intrusive containers.

## Intrusive Container Customizations

The intrusive container implementation has 6 customization points.

| Operation                                                 | Description                                                          |
| --------------------------------------------------------- | -------------------------------------------------------------------- |
| Tag::node_type(di::InPlaceType<T>) -> Tag::Node           | Get the correct node type for a given T.                             |
| Tag::down_cast(di::InPlaceType<T>, Tag::Node& node) -> T& | Cast from a node type to the underlying T value.                     |
| Tag::is_sized(di::InPlaceType<T>) -> bool                 | Determines whether the container offers an O(1) size function.       |
| Tag::always_store_tail(di::InPlaceType<T>) -> bool        | Determines whether a ForwardList offers O(1) back() and push_back(). |
| Tag::did_insert(IntrusiveContainer&, Tag::Node& node)     | Hook which intrusive container calls just after inserting the node.  |
| Tag::did_remove(IntrusiveContainer&, Tag::Node& node)     | Hook which intrusive container calls just after removing the node.   |

Notice, there are 2 hooks which are called when inserting and removing elements, respectively. This is used so that
reference-counted objects can take a reference on insertion and drop a reference on removal. The `did_remove` hook will
be used by the owning containers to delete the object directly. As an implementation note, the last template parameter
of the intrusive containers is the IntrusiveContainer type to pass the hooks. This allows user code to have the hook
called with their own subclass and node type.

The `down_cast` method is used to convert from an internal node to a value of type `T`. This is needed so that the
container's iterator will function correctly.

Additionally, each `Tag` passed to the container has an associated `Node` type, which is required to conform to the
container's requirements. This means that the `Node` type must either be the container's node base class, or the `Node`
type must publically inherit from it. Internally, intrusive containers will downcast their internal pointers to the
`Node` type before calling these customizations.

The `Tag` also can customize whether or not the intrusive container stores a size. By standard C++, the owning
list type should store the list's size. However, there is no need to store the size unless it is specifically needed.

In addition, the `Tag` can custommize whether or a `ForwardList` will store a pointer to the tail. This allows providing
a `.back()` and `.push_back()` functions, which makes the container usable with the `di::Queue` adapter. This is enabled
by default for normal intrusive containers, but by standard C++, the owning variant will not provide this functionality.

## Using the IntrusiveList class

Types which want to be added to an `IntrusiveList` can use the following snippet.

```cpp
class MyType : public di::IntrusiveListElement<> {};
```

```cpp
static void test() {
    auto my_list = di::IntrusiveList<MyType> {};
    auto x = MyType {};
    my_list.push_back(x);
}
```

To create a custom type, declare it like so and then inherit from the correct list element helper.

```cpp
struct MyTag : di::IntrusiveListTag<MyTag> {};

class MyType : public di::IntrusiveListNode<MyTag> {};
```

```cpp
static void test() {
    auto my_list = di::IntrusiveList<MyType, MyTag> {};
    auto x = MyType {};
    my_list.push_back(x);
}
```

As another example, to create a Type which needs shared ownership, a custom Tag can be used. In the future, there could
even be a helper tag-type to implement this pattern. Also, under this approach, there needs to be some action which
drops the last reference to `MyType`, since its initial reference count is 1.

```cpp
struct MyTag;

class MyType : public di::IntrusiveListNode<MyTag>, di::IntrusiveRefCount<MyType> {};

struct MyTag : di::IntrusiveListTag<MyTag> {
    using Node = di::IntrusiveListTag<MyTag>::Node;

    constexpr static void did_insert(di::InPlaceType<MyType> t, Node& node) {
        auto& value = MyTag::down_cast(t, node);
        auto to_leak = di::Arc<MyType>(node, di::adopt_object);
        (void) to_leak.release();
    }

    constexpr static void did_remove(di::InPlaceType<MyType> t, Node& node) {
        auto& value = MyTag::down_cast(t, node);
        auto to_drop = di::Arc<MyType>(node, di::retain_object);
    }
};
```
