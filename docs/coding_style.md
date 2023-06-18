# Coding Style

## Goals

The coding style offers 2 goals: conistency and quality. Consistency is important for readability, and quality is about
writing code which is both performant and correct. Consistency is acheived by using a linter, formatter, and the same
set of core libraries for all code. Quality is acheived by designing code to follow certain principles.

## Principles for Quality

1. **Safe by Construction**: The primitive elements of the system _must_ be safe. This includes temporal and spatial
   memory safety, thread safety, and type safety. When creating code using the basic elements, since the primitives
   themselves are safe, complex code will be as well. Hence the system is safe by construction.
2. **Model Problems with Types**: Leveraging types to model problems allows the compiler to check for correctness. The
   more code can rely on the compiler to check for correctness, the better.

### Safe by Construction

The system is safe by construction, because the primitives are safe. This means that the primitives are designed to be
safe, and then all other code is built on top of them. Unfortunately, the c++ language itself is fundamentally unsafe,
so simply using the primitives is not enough. We need to impose additional restrictions on the code to ensure it is
safe.

#### Temporal Memory Safety

The only restriction needed to ensure temporal memory safety is to **never store raw pointers or references in
datatypes**. This is because raw pointers and references can be invalidated, and hence are not safe. Instead, we use
reference counted pointers, or owned pointers. These pointers are safe, because they ensure the lifetime of the object.

However, it is necessary to use raw pointers or references when implementing primitives. This includes asynchronous
receivers, which typically store a pointer to an asynchronous operation state. This is safe, because the operation state
is immovable, and the machinery ensures that the operation state is not destroyed until the receiver gets a completion.

Another such primitive is iterators, which store a pointer to the underlying data. Iterators fail to be safe by
construction, but they must be used to implement generic algorithms. The standard range based algorithms improve the
situation by not returning iterators when passed rvalue references to non-borrowed containers. So using these APIS is
fairly safe, but it is still possible to misuse them. When downstream code needs to use iterators, it should be done
immediately, and not stored in a data structure. When elements need to be removed, use algorithms like `di::erase` or
`di::erase_if` to remove them, instead of writing the loop manually.

This leads to the hole in the attempt to avoid storing references: **datatypes can have reference semantics**. Iterators
as well as non-owning views (like di::StringView or di::Span) have reference semantics. This means that they are safe to
accept as a parameter, but not safe to store, since they do not own their underlying data.

Are refernece types safe to return from a function? In general, no. If the caller is not careful, they invalidate their
refernece. However, due to the prevalence of object getters which return references, this must be allowed. Luckily,
since we cannot actually store these references, invalid uses are limited to the lifetime of the function call. This
reduces the safety problem from a global one to a local one, which is much easier to deal with. However, there are still
problems with this approach. For example, here is an easy way to create a dangling reference:

```cpp
// A type which has a reference getter.
struct Fob {
public:
    di::StringView name() const { return m_name; }

private:
    di::String m_name;
    int m_value;
};

auto list = di::vector<Fob> { /* ... */ };

// Get a reference member to an item in the list:
auto first_name = list.front().value().name();

// Remove all items with the same name:
di::erase_if(list, [](auto& fob) { return fob.name() == first_name; });
```

This code is unsafe, because the reference returned by `name()` is invalidated when the first item is removed from the
list. Since it is still referenced by the predicate, this is a use-after-free. The problem here actually stems from the
fact that we implicitly took a reference to the first item in the list, but did not protect this using a smart pointer.

There are actually 3 ways to fix this code:

1. Store `di::Arc<Fob>` in the list. This has the downside of requiring a heap allocation, but conceptually demonstrates
   that the reference is owned.
2. Copy the item out of the list before using it. This is the simplest solution, but incurs a copy.
3. Use `di::partition` to partition the list, and then remove the second partition. This is the most efficient solution,
   but is problem specific and non-obvious.

As is normally the case with c++, we can make the code safe, but it requires some thought. We can even acheive optimal
performance, but it will be very challenging.

It turns out that the problem is not the fact that references are returned, but the model of the problem is not safe by
construction. The problem is that the list is not safe, because we both store the items directly, and plan on mutating
it later. If we need to mutate/delete items at random, we should use a `di::vector<di::Arc<Fob>>` instead. This is safe,
and accounts for the fact that we're going to randomly remove items. If we need to mutate the list, but not remove
items, we could almost use a `di::vector<Fob>`, but we need to be careful to not store references to the items.
Unfortunately, when adding elements to a vector, it can reallocate, which invalidates all references. What's worse if
that if the `di::String` type utilizes small string optimization, string views too it will be invalidated as well, but
only if the string is small. This is a very subtle problem, and is the reason why `di::StringView` is not safe to store.
In such a case, it would perhaps be better to use a stable container, such as the proposed `std::hive` container. This
does not exist yet, but could be a better idea than storing `di::Arc<Fob>`.

The conclusion here is that when modelling a problem, we must understand the program state, and know if and how it can
change. If we really have a collection of items which is constantly changing, these items **must** be stored as smart
pointers. On the other hand, having such a collection implies the code is improperly designed. It would be optimal for
data to only flow downward, and not allow random mutations. This is not always possible, but it is a good goal to strive
for. However, especially in the Iris kernel, which is reentrant due to system calls, this almost never happens. As such,
data structures in the kernel are almost always reference counted.

## Stylistic Choices

In general, the project tries to use the newest C++ features available, once they are supported by both clang and gcc.
This also involves trying to avoid things considered bad practice, like macros and raw `new`/`delete`.

The project does not use the C++ standard library, as the Iris kernel cannot use it. Instead, it uses the custom `di`
library, which attempts to mimic the standard library, but replaces exceptions with `std::expected` (called
`di::Expected`).

## Format Style

The project has a `.clang-format` file that can be used to format the code. Any code not formatted correctly will be
rejected by in CI. The only interesting thing to note is that the latest version (clang-format-16) is used, which
enables controlling the order of qualifiers. This is useful for the `const` qualifier, which is placed after the type
(east-const), which is the preferred style.

For JSON, YAML, and Markdown files, the project uses [prettier](https://prettier.io/) to format the files. This is also
enforced by CI. In particular, JSON files are not allowed to have any comments, even in place where they would be
allowed normally (like in VS Code settings.json).

Finally, cmake files are formatted using [cmake-format](https://github.com/cheshirekow/cmake_format), which is again
enforced by CI.

When using the provided dev container, all of these tools are installed, and when using vscode, extensions are installed
to enable autoformatting.

## Linting

The project supports linting using [clang-tidy](https://clang.llvm.org/extra/clang-tidy/). This is not currently
enforced by CI, but it is recommended to ensure newly written code passes the linter. The main reason it is not enforced
is that `clang-analyzer` checks take an extremely long time to run, and seemingly have a large number of false
positives.

In the future, we should consider fine tuning the configuration to avoid these issues, and then enable it in CI. It
would also be desirable to use other linters, such as [cppcheck](https://cppcheck.sourceforge.io/).
