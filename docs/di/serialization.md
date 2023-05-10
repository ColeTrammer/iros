# Serialization

## Purpose

The library aims to provide an extremely convient way to serialize and deserialize objects, leveraging static reflection
to provide a simple interface.

## Usage

Consider a normal type `MyType` which has static reflection added. This can be serialized as follows:

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
#include <di/serialization/json_serializer.h>

auto x = MyType { 1, 2, 3 };

// Serialize to JSON string:
auto string = TRY(di::to_json_string(x));
// or...
auto string = TRY(di::serialize_string(di::json_format, x));

// Serialize to JSON file (with pretty printing):
auto file = TRY(dius::open_sync("file.json", dius::OpenMode::WriteOnly | dius::OpenMode::Create));
TRY(di::serialize_json(file, x, di::JsonSerializerConfig().pretty()));
// or ...
TRY(di::serialize(di::json_format, file, x, di::JsonSerializerConfig().pretty()));
// or ...
auto serializer = di::JsonSerializer(di::move(file), di::JsonSerializerConfig().pretty());
TRY(di::serialize(serializer, x));
```

The library also supports deserialization using a mostly symmetric interface. For example:

```cpp
#include <di/serialization/json_deserializer.h>

// Deserialize from JSON string:
auto x = TRY(di::from_json_string<MyType>("{ x: 1, y: 2, z: 3 }"_sv));
// or...
auto x = TRY(di::deserialize_string<MyType>(di::json_format, "{ x: 1, y: 2, z: 3 }"_sv));

// Deserialize from JSON file:
auto file = TRY(dius::open_sync("file.json", dius::OpenMode::ReadOnly));
auto x = TRY(di::deserialize_json<MyType>(file));
// or...
auto x = TRY(di::deserialize<MyType>(di::json_format, file));
// or...
auto deserializer = di::JsonDeserializer(di::move(file));
auto x = TRY(di::deserialize(deserializer));
```

Using the JSON serializer, enumerations can also be serialized to strings using static reflection. Additionally, users
can provide custom serialization behavior by overriding the `di::serialize_metadata` function. For instance:

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

This metadata search is handled by the `di::serialize` function, which evaluates the metadata and then calls the
provided serializer. This dispatching mechanism ensures that serializers will have consistent behavior across all
implementations.

Additionally, `di::serialize()` is a tag-invoke CPO, so the entire serialization mechanism can be customized on a
per-type basis if needed.

For example,

```cpp
struct UUID {
    // Returns fixed-size string representation of the UUID.
    auto to_string() const;

    // Custom serialization. For JSON, we would want a string, but for binary formats we would want a fixed-size array.
    friend auto tag_invoke(di::Tag<di::serialize>, di::JsonFormat, auto& serializer, UUID const& value) {
        return serializer.serialize_string(value.to_string());
    }

    di::Array<byte, 16> bytes;
};
```

The deserialization mechanism has equivalent customization points, like `di::deserialize_metadata` and
`di::deserialize`. However, they take `di::InPlaceType<T>` as arguments instead of the actual value to be serialized.
Additionally, `di::deserialization_metadata` will fallback to `di::serialize_metadata` if no deserialization metadata
is provided.

## Custom Serialization Formats

The library is designed to be extensible to support multiple serialization formats in the future. A serialization format
is modelled by the `di::SerializationFormat` concept, which is defined as follows:

```cpp
template<typename T, typename Writer = di::AnyRef<io::Writer>, typename... Args>
concept SerializationFormat = requires(T format, Writer&& writer, Args&&... args) {
    serialization::serializer(format, di::forward<Writer>(writer), di::forward<Args>(args)...);
};
```

As shown, a serialization format must provide an implementation of the `serializer` function which takes a writer, and
returns a bound serializer. This can be done by providing a static function `serializer`. This should be valid for any
type of writer, but since c++ concepts cannot universality, a type-erased Writer is checked. The serializer must be a
type which models the `di::Serializer` concept, which is defined as follows:

```cpp
template <typename T>
concept Serializer = requires(T& serializer, DummyType value, DummyMetadata metadata) {
    typename SerializationFormat;
    { serializer.serialize(value, metadata) } -> ConvertibleTo<di::Result<void>>;
    { serializer.writer() } -> di::Impl<Writer>;
    di::as_const(serializer).writer();
    di::move(serializer).writer();
};
```

The metadata object passed to the serialization is either a list of fields or a atom, depending on the type being
serialized. Different serializers may be able to serialize different values, so the second constraint on `Serializer` is
not enforced. Instead, the important concept is one that considers both the serializer and the type being serialized:

```cpp
template<typename T, typename S>
concept Serializable =
    di::Serializer<S> && requires(S&& serializer, T&& value) { di::serialize(serializer, value); };
```

## Custom Deserialization Formats

The deserialization mechanism is similar to the serialization mechanism, but with a few key differences. First, the
`di::Deserializer` concept replaces the `writer()` function with a `reader()` function. Second, the `di::Deserializer`
concept is not constrained by the `SerializationFormat` concept. Instead, it is constrained by the
`DeserializationFormat` concept, which is defined as follows:

```cpp
template<typename T, typename Reader = di::AnyRef<io::Reader>, typename... Args>
concept DeserializationFormat = requires(T format, Reader&& reader, Args&&... args) {
    serialization::deserializer(format, di::forward<Reader>(reader), di::forward<Args>(args)...);
};
```

To be a useful `di::Deserializer`, the type must overload the `deserialize()` function to accept an `InPlaceType<T>` to
be deserialized. Optionally, it can also accept a metadata object for the type being deserialized.
