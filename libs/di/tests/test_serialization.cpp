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
            return object.serialize_key("key"_sv, [&](auto& serializer) {
                return serializer.serialize_string("value"_sv);
            });
        });

        auto result = di::move(serializer).writer().output();
        ASSERT_EQ(result, R"({"key":"value"})"_sv);
    }

    {
        auto writer = di::StringWriter {};
        auto serializer = di::JsonSerializer(di::move(writer));

        *serializer.serialize_object([&](auto& object) -> di::meta::WriterResult<void, decltype(writer)> {
            TRY(object.serialize_key("key"_sv, [&](auto& serializer) {
                return serializer.serialize_number(42);
            }));
            TRY(object.serialize_key("key2"_sv, [&](auto& serializer) {
                return serializer.serialize_string("value"_sv);
            }));
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

TESTC(serialization, json_basic)
}
