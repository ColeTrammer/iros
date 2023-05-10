#include <di/reflect/prelude.h>
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

    {
        auto r = *di::from_json_string<di::json::Array>(R"([1, 2, 3])"_sv);
        ASSERT_EQ(r[0], 1);
        ASSERT_EQ(r[1], 2);
        ASSERT_EQ(r[2], 3);
    }
    {
        auto r = *di::from_json_string<di::json::Object>(R"({"hello": 42, "world": 43})"_sv);
        ASSERT_EQ(r["hello"_sv], 42);
        ASSERT_EQ(r["world"_sv], 43);
    }
}

struct MyType {
    int x;
    int y;
    int z;
    bool w;
    di::String a;

    bool operator==(MyType const& other) const = default;

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
    di::Vector<int> array;
    di::TreeMap<di::String, int> map;
    MyEnum my_enum;

    bool operator==(MySuperType const& other) const = default;

    constexpr friend auto tag_invoke(di::Tag<di::reflect>, di::InPlaceType<MySuperType>) {
        return di::make_fields(di::field<"my_type", &MySuperType::my_type>, di::field<"array", &MySuperType::array>,
                               di::field<"map", &MySuperType::map>, di::field<"my_enum", &MySuperType::my_enum>);
    }
};

constexpr void json_reflect() {
    {
        auto r = *di::from_json_string<MyEnum>(R"("Foo")"_sv);
        auto e = MyEnum::Foo;
        ASSERT_EQ(r, e);
    }
    {
        auto r = *di::from_json_string<MyEnum>(R"("Bar")"_sv);
        auto e = MyEnum::Bar;
        ASSERT_EQ(r, e);
    }
    {
        auto r = *di::from_json_string<MyEnum>(R"("Baz")"_sv);
        auto e = MyEnum::Baz;
        ASSERT_EQ(r, e);
    }
    {
        auto r = *di::from_json_string<MyType>(R"({
        "x": 1,
        "y": 2,
        "z": 3,
        "w": true,
        "a": "hello"
    })"_sv);

        auto e = MyType { 1, 2, 3, true, "hello"_sv.to_owned() };
        ASSERT_EQ(r, e);
    }
    {
        auto r = *di::from_json_string<MySuperType>(R"({
        "my_type": {
            "x": 1,
            "y": 2,
            "z": 3,
            "w": true,
            "a": "hello"
        },
        "array": [1, 2, 3],
        "map": {
            "hello": 1,
            "world": 2
        },
        "my_enum": "Bar"
    })"_sv);

        auto e =
            MySuperType { MyType { 1, 2, 3, true, "hello"_sv.to_owned() }, di::Array { 1, 2, 3 } | di::to<di::Vector>(),
                          di::Array { di::Tuple { "hello"_sv.to_owned(), 1 }, di::Tuple { "world"_sv.to_owned(), 2 } } |
                              di::as_rvalue | di::to<di::TreeMap>(),
                          MyEnum::Bar };
        ASSERT_EQ(r, e);
    }
}

TESTC(deserialization, json_value)
TESTC_CLANG(deserialization, json_literal)
TESTC_CLANG(deserialization, json_reflect)
}
