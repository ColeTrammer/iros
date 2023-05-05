#include <di/io/interface/writer.h>
#include <di/io/prelude.h>
#include <di/io/string_writer.h>
#include <di/reflect/prelude.h>
#include <di/serialization/json_serializer.h>
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
        return di::make_tuple(di::field<"x", &MyType::x>, di::field<"y", &MyType::y>, di::field<"z", &MyType::z>,
                              di::field<"w", &MyType::w>, di::field<"a", &MyType::a>);
    }
};

struct MySuperType {
    MyType my_type;
    di::Array<int, 3> array;
    di::Array<di::Tuple<di::StringView, int>, 3> map;

    constexpr friend auto tag_invoke(di::Tag<di::reflect>, di::InPlaceType<MySuperType>) {
        return di::make_tuple(di::field<"my_type", &MySuperType::my_type>, di::field<"array", &MySuperType::array>,
                              di::field<"map", &MySuperType::map>);
    }
};

constexpr void json_reflect() {
    {
        auto writer = di::StringWriter {};
        auto serializer = di::JsonSerializer(di::move(writer));

        auto const x = MyType { 1, 2, 3, true, "hello"_sv };
        *serializer.serialize(x);

        auto result = di::move(serializer).writer().output();
        ASSERT_EQ(result, R"({"x":1,"y":2,"z":3,"w":true,"a":"hello"})"_sv);
    }

    {
        auto writer = di::StringWriter {};
        auto serializer = di::JsonSerializer(di::move(writer));

        auto const x = MySuperType { MyType { 1, 2, 3, true, "hello"_sv },
                                     { 1, 2, 3 },
                                     { di::Tuple { "a"_sv, 1 }, di::Tuple { "b"_sv, 2 }, di::Tuple { "c"_sv, 3 } } };

        *serializer.serialize(x);

        auto result = di::move(serializer).writer().output();
        ASSERT_EQ(
            result,
            R"({"my_type":{"x":1,"y":2,"z":3,"w":true,"a":"hello"},"array":[1,2,3],"map":{"a":1,"b":2,"c":3}})"_sv);
    }
}

TESTC(serialization, json_basic)
TESTC(serialization, json_pretty)
TESTC(serialization, json_reflect)
}
