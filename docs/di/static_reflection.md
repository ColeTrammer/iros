# Static Reflection

## Purpose

Several programming utilities can become greatly simplified by using a reflection system. This allows code to introspect
a type at compile time, and use this information to provide functionality. For instance, this can be used to implement
automatic serialization and deserialization, or to implement enable introspection of objects at runtime. Specifically,
this can be used to implement overloads of `di::hash`, implement `di::format` for a type, implement `di::parse` for a
type, and so on.

## Note on C++

In the future, there will presumably be static reflection in standard C++. However, this is not yet available, but is
incredibly useful. This library provides a static reflection system which can be used in the meantime.

Traditionally, these sorts of systems are implemented using macros. However, this library uses a different approach,
which is possible due to the use of C++ 20 features (namely, string non-type template parameters).

## Usage

To enable static reflection for a given type, provide a hidden-friend overload of `di::reflect` for the type. This can
be done as follows:

```cpp
#include "di/reflect/prelude.h"

struct MyType {
    int x;
    int y;
    int z;

    constexpr friend auto tag_invoke(di::Tag<di::reflect>, di::InPlaceType<MyType>) {
        return di::make_fields(
            di::field<"x", &MyType::x>,
            di::field<"y", &MyType::y>,
            di::field<"z", &MyType::z>
        );
    }
};

enum class MyEnum {
    A,
    B,
    C
};

constexpr auto tag_invoke(di::Tag<di::reflect>, di::InPlaceType<MyEnum>) {
    using enum MyEnum;
    return di::make_enumerators(
        di::enumerator<"A", A>,
        di::enumerator<"B", B>,
        di::enumerator<"C", C>
    );
}
```

If using a macro based approach to reflection, this would be equivalent to the following:

```cpp
struct MyType {
    int x;
    int y;
    int z;

    DI_RELECT(MyType, x, y, z);
};

DI_DECLARE_ENUM(MyEnum, A, B, C);
```

This is definitely shorter, but it is also less flexible. For instance, it is not possible to use a different name for
the fields (which could useful for JSON serialization, or because member variables have the 'm\_' prefix). Additionally,
the macro approach would require preprocessor iteration magic, which means there will be an upper bound to the number of
fields which can be supported. And syntax errors when using the macro will result in utterly incomprehensible error
messages.

For instance, here is an example of reflecting a class type with private member variables. Since the reflection is
implemented using hidden friend functions, it is possible to access private members.

```cpp
class MyClass {
    int m_x;
    int m_y;
    int m_z;

public:
    constexpr MyClass(int x, int y, int z) : m_x(x), m_y(y), m_z(z) {}

    constexpr int x() const { return m_x; }
    constexpr int y() const { return m_y; }
    constexpr int z() const { return m_z; }

    constexpr friend auto tag_invoke(di::Tag<di::reflect>, di::InPlaceType<MyClass>) {
        return di::make_fields(
            di::field<"x", &MyClass::m_x>,
            di::field<"y", &MyClass::m_y>,
            di::field<"z", &MyClass::m_z>
        );
    }
};

enum class MyEnum {
    A,
    B,
    C
};

constexpr auto tag_invoke(di::Tag<di::reflect>, di::InPlaceType<MyEnum>) {
    using enum MyEnum;
    return di::make_enumerators(
        di::enumerator<"MYENUM_A", A>,
        di::enumerator<"MYENUM_B", B>,
        di::enumerator<"MYENUM_C", C>
    );
}
```

## Internal Representation

The internal representation of a reflected type is an `di::reflection::Fields` object. Each type member in the list
corresponds to a field. Each field is a `di::reflection::Field` object, which contains a name and a pointer to the
member. The `Fields` object inherits from `di::Tuple`, and is thus easily convertible to a type-list. This lets compile
time code use the existing type-list meta-programming tools. And at no additional cost, the `Fields` object can be used
like a tuple.

Enumerations are also supported. For this case, the reflect type is an `di::reflection::Enumerators` object. This is
similar to the `di::reflection::Fields` object, but instead of containing a list of fields, it contains a list of
enumerators. Each enumerator is a `di::reflection::Enumerator` object, which contains a name and a value. These can also
be interacted with normally as a tuple.

These types have their information fully encoded in the type system, which means they effectively store no data. The
field class simply looks as follows:

```cpp
template<di::FixedString field_name, auto field_pointer>
requires(di::concepts::MemberObjectPointer<decltype(field_pointer)>)
struct Field {
    constexpr static auto name = field_name;
    constexpr static auto pointer = field_pointer;

    using Object = di::meta::MemberPointerClass<decltype(pointer)>;
    using Type = di::meta::MemberPointerValue<decltype(pointer)>;

    template<typename T>
    requires(di::concepts::Invocable<decltype(pointer), T>)
    constexpr static decltype(auto) get(T&& object) {
        return di::invoke(pointer, di::forward<T>(object));
    }
};
```

### Atoms

The `di::reflection::Atom` class is used to represent a primitive type which is not divisible into fields. For instance,
integers, strings, and booleans are atoms. This enables reflection of these types, and will allow classes which are
semantically equivalent to be treated as equivalent. For instance, an `int` and a `di::StrongInt<int, MyTag>` can both
reflect as an integer, and can be treated as equivalent.

## Accessing Reflection Information

The `di::reflect` function can be used to access the reflection information for a type. Since this is a function, it
will return the reflection information as a value which models `di::ReflectionValue`. For example, calling
`di::reflect(mytype_instance)` will return the custom `di::Fields` object the type defines. If a type is needed,
`di::meta::Reflect<MyType>` can be used instead. Since `di::reflect()` can also return an `di::reflection::Atom` or
`di::reflection::Enumerators` object, it is necessary to constrain functions on `di::ReflectableToFields` to in certain
cases.

This can be used to implement various utilities. For instance, the following function can be used to print every member
of a type:

```cpp
static void print_fields(di::ReflectableToFields auto const& object) {
    di::tuple_for_each([&](auto field) {
        dius::println("{}: {}", field.name, field.get(object));
    }, di::reflect(object));
}
```

Another example is hashing a type:

```cpp
static void hash_fields(di::Hasher auto& hasher, di::ReflectableToFields auto const& object) {
    di::tuple_for_each([&](auto field) {
        di::hash_write(hasher, field.get(object));
    }, di::reflect(object));
}
```

For enums, we can get the name of an enumerator:

```cpp
// NOTE: this is already defined by the library as `di::enum_to_string()`.
constexpr auto enum_to_string(di::ReflectableToEnumerators auto value) {
    auto result = "Invalid"_sv;
    di::tuple_for_each([&](auto enumerator) {
        if (enumerator.value == value) {
            // NOTE: the strings in this library are compile-time values (with fixed length), so we need to convert them
            // to a normal string view.
            result = di::container::fixed_string_to_utf8_string_view<enumerator.name>();
        }
    }, di::reflect(value));
    return result;
}
```

Since the reflection information is stored in a tuple, it can be easily accessed without template metaprogramming. This
should make it easier to implement various utilities.

## Uses in library

Providing static reflection information for a type enables several implementations in the library automatically. The
current list is as follows:

1. `di::format()` and `di::to_string()` will use the reflection information to print the contents of a type or enum.
2. `di::hash()` will use the reflection information to hash the contents of a type.
3. `di::serialize()` will use the reflection information to serialize the contents of a type or enum.
4. `di::parse()` will use the reflection information to parse an enum from a string.

## Limitations

The current implementation has a few limitations. The main one is that it does not support inheritance. In some cases,
is useful for a type to inherit the reflection information of a base type. For instance, a message type might inherit
the reflection information of a base message type. Currently, this would require manually copying the reflection
information from the base type to the derived type. This is not ideal, but it is not too bad.

The library also does not support reflecting member functions, but this can be added if a need arises.
