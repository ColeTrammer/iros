#include <di/io/interface/writer.h>
#include <di/io/prelude.h>
#include <di/io/string_writer.h>
#include <di/io/vector_writer.h>
#include <di/reflect/prelude.h>
#include <di/serialization/binary_serializer.h>
#include <di/serialization/json_value.h>
#include <dius/test/prelude.h>

namespace serialization {
constexpr void json_basic() {
    static_assert(di::Impl<di::StringWriter<>, di::Writer>);

    {
        auto writer = di::StringWriter {};
        auto serializer = di::JsonSerializer(di::move(writer));

        *serializer.serialize_object([&](auto& object) {
            return object.serialize_string("key"_sv, "value"_sv);
        });

        auto result = di::move(serializer).writer().output();
        ASSERT_EQ(result, R"({"key":"value"})"_sv);
    }

    {
        auto writer = di::StringWriter {};
        auto serializer = di::JsonSerializer(di::move(writer));

        *serializer.serialize_object([&](auto& object) -> di::meta::WriterResult<void, decltype(writer)> {
            TRY(object.serialize_number("key"_sv, 42));
            TRY(object.serialize_string("key2"_sv, "value"_sv));
            return {};
        });

        auto result = di::move(serializer).writer().output();
        ASSERT_EQ(result, R"({"key":42,"key2":"value"})"_sv);
    }

    {
        auto writer = di::StringWriter {};
        auto serializer = di::JsonSerializer(di::move(writer));

        *serializer.serialize_array([&](auto& serializer) -> di::meta::WriterResult<void, decltype(writer)> {
            TRY(serializer.serialize_number(42));
            TRY(serializer.serialize_string("value"_sv));
            return {};
        });

        auto result = di::move(serializer).writer().output();
        ASSERT_EQ(result, R"([42,"value"])"_sv);
    }
}

constexpr void json_pretty() {
    {
        auto writer = di::StringWriter {};
        auto serializer = di::JsonSerializer(di::move(writer), di::JsonSerializerConfig().pretty());

        *serializer.serialize_object([&](auto& object) -> di::meta::WriterResult<void, decltype(writer)> {
            TRY(object.serialize_number("key"_sv, 42));
            TRY(object.serialize_string("key2"_sv, "value"_sv));
            return {};
        });

        auto result = di::move(serializer).writer().output();
        ASSERT_EQ(result, R"({
    "key": 42,
    "key2": "value"
})"_sv);
    }

    {
        auto writer = di::StringWriter {};
        auto serializer = di::JsonSerializer(di::move(writer), di::JsonSerializerConfig().pretty().indent_width(2));

        *serializer.serialize_object([&](auto& object) -> di::meta::WriterResult<void, decltype(writer)> {
            TRY(object.serialize_number("key"_sv, 42));
            TRY(object.serialize_string("key2"_sv, "value"_sv));
            TRY(object.serialize_array("key3"_sv,
                                       [&](auto& serializer) -> di::meta::WriterResult<void, decltype(writer)> {
                                           TRY(serializer.serialize_number(42));
                                           TRY(serializer.serialize_string("value"_sv));
                                           return {};
                                       }));
            TRY(object.serialize_object("key4"_sv, [&](auto&) -> di::meta::WriterResult<void, decltype(writer)> {
                return {};
            }));
            return {};
        });

        auto result = di::move(serializer).writer().output();
        ASSERT_EQ(result, R"({
  "key": 42,
  "key2": "value",
  "key3": [
    42,
    "value"
  ],
  "key4": {}
})"_sv);
    }
}

struct MyType {
    int x;
    int y;
    int z;
    bool w;
    di::StringView a;

    constexpr friend auto tag_invoke(di::Tag<di::reflect>, di::InPlaceType<MyType>) {
        return di::make_fields(di::field<"x", &MyType::x>, di::field<"y", &MyType::y>, di::field<"z", &MyType::z>,
                               di::field<"w", &MyType::w>, di::field<"a", &MyType::a>);
    }
};

enum class MyEnum { Foo, Bar, Baz };

constexpr auto tag_invoke(di::Tag<di::reflect>, di::InPlaceType<MyEnum>) {
    using enum MyEnum;
    return di::make_enumerators(di::enumerator<"Foo", Foo>, di::enumerator<"Bar", Bar>, di::enumerator<"Baz", Baz>);
}

struct MySuperType {
    MyType my_type;
    di::Array<int, 3> array;
    di::Array<di::Tuple<di::StringView, int>, 3> map;
    MyEnum my_enum;
    di::Optional<int> optional;

    constexpr friend auto tag_invoke(di::Tag<di::reflect>, di::InPlaceType<MySuperType>) {
        return di::make_fields(di::field<"my_type", &MySuperType::my_type>, di::field<"array", &MySuperType::array>,
                               di::field<"map", &MySuperType::map>, di::field<"my_enum", &MySuperType::my_enum>,
                               di::field<"optional", &MySuperType::optional>);
    }
};

constexpr void json_reflect() {
    {
        auto const x = MyType { 1, 2, 3, true, "hello"_sv };
        auto result = di::to_json_string(x);
        ASSERT_EQ(result, R"({"x":1,"y":2,"z":3,"w":true,"a":"hello"})"_sv);
    }
    {
        auto const x = MySuperType { MyType { 1, 2, 3, true, "hello"_sv },
                                     { 1, 2, 3 },
                                     { di::Tuple { "a"_sv, 1 }, di::Tuple { "b"_sv, 2 }, di::Tuple { "c"_sv, 3 } },
                                     MyEnum::Bar,
                                     di::nullopt };

        auto result = di::to_json_string(x, di::JsonSerializerConfig().pretty().indent_width(4));
        ASSERT_EQ(result, R"({
    "my_type": {
        "x": 1,
        "y": 2,
        "z": 3,
        "w": true,
        "a": "hello"
    },
    "array": [
        1,
        2,
        3
    ],
    "map": {
        "a": 1,
        "b": 2,
        "c": 3
    },
    "my_enum": "Bar"
})"_sv);
    }
    {
        auto const x = MySuperType { MyType { 1, 2, 3, true, "hello"_sv },
                                     { 1, 2, 3 },
                                     { di::Tuple { "a"_sv, 1 }, di::Tuple { "b"_sv, 2 }, di::Tuple { "c"_sv, 3 } },
                                     MyEnum::Bar,
                                     5 };

        auto result = di::to_json_string(x, di::JsonSerializerConfig().pretty().indent_width(4));
        ASSERT_EQ(result, R"({
    "my_type": {
        "x": 1,
        "y": 2,
        "z": 3,
        "w": true,
        "a": "hello"
    },
    "array": [
        1,
        2,
        3
    ],
    "map": {
        "a": 1,
        "b": 2,
        "c": 3
    },
    "my_enum": "Bar",
    "optional": 5
})"_sv);
    }
}

constexpr void json_value() {
    auto r1 = di::to_json_string(di::json::null);
    ASSERT_EQ(r1, "null"_sv);

    auto r2 = di::to_json_string(di::json::Bool(true));
    ASSERT_EQ(r2, "true"_sv);

    auto x3 = di::json::Value();
    x3["key"_sv] = true;
    auto r3 = di::to_json_string(x3);
    ASSERT_EQ(r3, R"({"key":true})"_sv);
}

constexpr void binary() {
    auto do_test = [](auto const& value, auto const& expected) {
        auto writer = di::VectorWriter<>();
        ASSERT(di::serialize_binary(writer, value));
        ASSERT(di::container::equal(writer.vector(), expected));
    };

    do_test(42, di::Array { 42_b, 0_b, 0_b, 0_b });
    do_test(di::make_tuple(42, 42), di::Array { 42_b, 0_b, 0_b, 0_b, 42_b, 0_b, 0_b, 0_b });
    do_test("hello"_sv, di::Array { 5_b, 0_b, 0_b, 0_b, 0_b, 0_b, 0_b, 0_b, 'h'_b, 'e'_b, 'l'_b, 'l'_b, 'o'_b });

    auto variant = di::Variant<int, di::StringView>(42);
    do_test(variant, di::Array { 0_b, 0_b, 0_b, 0_b, 0_b, 0_b, 0_b, 0_b, 42_b, 0_b, 0_b, 0_b });

    variant = "hello"_sv;
    do_test(variant, di::Array { 1_b, 0_b, 0_b, 0_b, 0_b, 0_b,   0_b,   0_b,   5_b,   0_b,  0_b,
                                 0_b, 0_b, 0_b, 0_b, 0_b, 'h'_b, 'e'_b, 'l'_b, 'l'_b, 'o'_b });

    auto custom = MyType { 1, 2, 3, true, "hello"_sv };
    do_test(custom, di::Array { 1_b, 0_b, 0_b, 0_b, 2_b, 0_b, 0_b, 0_b, 3_b,   0_b,   0_b,   0_b,   1_b,
                                5_b, 0_b, 0_b, 0_b, 0_b, 0_b, 0_b, 0_b, 'h'_b, 'e'_b, 'l'_b, 'l'_b, 'o'_b });
}

TESTC(serialization, json_basic)
TESTC(serialization, json_pretty)
TESTC(serialization, json_reflect)
TESTC(serialization, json_value)
TESTC(serialization, binary)
}
