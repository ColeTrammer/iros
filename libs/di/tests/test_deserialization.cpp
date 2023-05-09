#include <di/serialization/json_deserializer.h>
#include <di/serialization/json_value.h>
#include <dius/test/prelude.h>

namespace deserialization {
constexpr void json_value() {
    auto x = di::json::Value {};
    ASSERT(x.is_null());
    auto r1 = di::visit(di::overload(
                            [](di::json::Null) {
                                return 1;
                            },
                            [](auto&&) {
                                return 0;
                            }),
                        di::as_const(x));
    ASSERT_EQ(r1, 1);

    x["hello"_sv] = 42;
    ASSERT(x.is_object());
    ASSERT_EQ(x.at("hello"_sv), 42);

    x.push_back(42);
    ASSERT(x.is_array());
    ASSERT_EQ(x.at(0), 42);

    ASSERT_LT(x[0], 43);
    ASSERT_EQ(x.size(), 1);
    ASSERT(!x.empty());

    x.insert_or_assign("hello"_sv, 43);
    ASSERT(x["hello"_sv].is_number());
    ASSERT_EQ(x.at("hello"_sv), 43);

    x.try_emplace("world"_sv, 44);
    ASSERT_EQ(x.at("world"_sv), 44);

    x["world"_sv] = di::create<di::json::Value>("value"_sv);
    x["world"_sv] = "value"_sv.to_owned();
    ASSERT_EQ(*x.at("world"_sv), "value"_sv);

    ASSERT_EQ(x.size(), 2);
    ASSERT(!x.empty());

    x.clear();
    ASSERT(x.is_object());
    ASSERT(x.empty());

    x["hello"_sv] = 42;
    x["world"_sv] = 43;
    ASSERT_EQ(di::to_string(x), R"({
    "hello": 42,
    "world": 43
})"_sv);
}

constexpr void json_literal() {
    auto object = R"( {
    "hello" : 32 , "world" : [ "x" , null ]
} )"_json;

    ASSERT_EQ(object["hello"_sv], 32);
    ASSERT_EQ(object["world"_sv][0], "x"_sv);
    ASSERT_EQ(object["world"_sv][1], di::json::null);
}

TESTC(deserialization, json_value)
TESTC_CLANG(deserialization, json_literal)
}
