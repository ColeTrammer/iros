# Serialization

## Purpose

The library aims to provide an extremely convient way to serialize and deserialize objects, leveraging static reflection
to provide a simple interface.

## Usage

Consider a normal type `MyType` which has static reflection added. This can be serialized and deserialized as follows:

```cpp
#include <di/reflect/prelude.h>

struct MyType {
    int x;
    int y;
    int z;

    constexpr friend auto tag_invoke(di::Tag<di::reflect>, di::InPlaceType<MyType>) {
        return di::make_tuple(
            di::field<"x", &MyType::x>,
            di::field<"y", &MyType::y>,
            di::field<"z", &MyType::z>
        );
    }
};
```

```cpp
auto x = MyType { 1, 2, 3 };

// Serialize to JSON string:
auto string = TRY(di::serialize_json_string(x));
// or...
auto string = TRY(di::serialize(x, di::json_format));

// Serialize to JSON file:
auto file = TRY(dius::open_sync("file.json", dius::OpenMode::WriteOnly | dius::OpenMode::Create));
TRY(di::serialize_json(x, file));
// or...
TRY(di::serialize(x, file, di::json_format));
```

Additionally, users can provide custom serialization behavior by overriding the `di::serialize_metadata` function. For
instance:

```cpp
#include <di/reflect/prelude.h>
#include <di/serialization/prelude.h>

struct MyType {
    int x_abc;
    int y_abc;
    int z_abc;

    constexpr friend auto tag_invoke(di::Tag<di::serialize_metadata>, di::InPlaceType<MyType>, di::InPlaceType<di::JsonFormat>) {
        return di::make_tuple(
            di::field<"xAbc", &MyType::x>,
            di::field<"zAbc", &MyType::z>
        );
    }
};
```

This allows overriding the names or only serializing a subset of the fields. This can be done on a per-format basis or
for all formats at once. When resolving how to serialize a type, the following order is used:

1. If the type has a `di::serialize_metadata` overload for the given format, use that.
2. If the type has a blanket `di::serialize_metadata` overload, use that.
3. If the type has a `di::reflect` overload, use that.

## Custom Serialization Formats

The library is designed to be extensible to support multiple serialization formats in the future. A serialization format
is modelled by the `di::SerializationFormat` concept, which is defined as follows:

```cpp
template <typename T>
concept SerializationFormat = requires(DummyWriter& writer, DummyObject& object) {
    T::serialize(writer, serialization_metadata(object), object);
};
```

As shown, a serialization format must provide a static `serialize` function which takes a writer, a metadata object, and
the actual object to serialize. The metadata object is a tuple of `di::Field` objects.

Additionally, each serialization format must define serialize overloads for primitive types, which do not have any
metadata. These include strings, integers, containers, and any other types which need to be serialized.
