#include <di/io/interface/writer.h>
#include <di/io/prelude.h>
#include <di/io/string_writer.h>
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

TESTC(serialization, json_basic)
TESTC(serialization, json_pretty)
}
