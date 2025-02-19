#include "di/container/hash/prelude.h"
#include "di/reflect/prelude.h"
#include "di/reflect/valid_enum_value.h"
#include "di/util/bitwise_enum.h"
#include "dius/test/prelude.h"

namespace reflect {
struct MyType {
    int x;
    int y;
    int z;

    constexpr friend auto tag_invoke(di::Tag<di::reflect>, di::InPlaceType<MyType>) {
        return di::make_fields<"MyType">(di::field<"x", &MyType::x>, di::field<"y", &MyType::y>,
                                         di::field<"z", &MyType::z>);
    }
};

class MyClass {
    int m_x;
    int m_y;
    int m_z;

public:
    constexpr MyClass(int x, int y, int z) : m_x(x), m_y(y), m_z(z) {}

    constexpr auto x() const -> int { return m_x; }
    constexpr auto y() const -> int { return m_y; }
    constexpr auto z() const -> int { return m_z; }

    constexpr friend auto tag_invoke(di::Tag<di::reflect>, di::InPlaceType<MyClass>) {
        return di::make_fields<"MyClass">(di::field<"x", &MyClass::m_x>, di::field<"y", &MyClass::m_y>,
                                          di::field<"z", &MyClass::m_z>);
    }
};

constexpr static void hash_fields(di::Hasher auto& hasher, di::ReflectableToFields auto const& object) {
    di::tuple_for_each(
        [&](auto field) {
            di::hash_write(hasher, field.get(object));
        },
        di::reflect(object));
}

constexpr static void basic() {
    static_assert(di::Reflectable<MyType>);

    auto x = MyType { 1, 2, 3 };

    auto hasher = di::DefaultHasher {};
    hash_fields(hasher, x);
    auto r1 = hasher.finish();

    hasher = di::DefaultHasher {};
    di::hash_write(hasher, x.x);
    di::hash_write(hasher, x.y);
    di::hash_write(hasher, x.z);
    auto ex1 = hasher.finish();

    ASSERT_EQ(r1, ex1);
    ASSERT_EQ(di::hash(x), ex1);
}

constexpr static void private_fields() {
    auto y = MyClass { 1, 2, 3 };

    auto hasher = di::DefaultHasher {};
    hash_fields(hasher, y);
    auto r2 = hasher.finish();

    hasher = di::DefaultHasher {};
    di::hash_write(hasher, y.x());
    di::hash_write(hasher, y.y());
    di::hash_write(hasher, y.z());
    auto ex2 = hasher.finish();

    ASSERT_EQ(r2, ex2);
}

constexpr static void format() {
    auto x = MyType { 1, 2, 3 };
    auto s = di::to_string(x);

    ASSERT_EQ(s, "MyType { x: 1, y: 2, z: 3 }"_sv);
}

enum class MyEnum { Foo, Bar, Baz };

constexpr static auto tag_invoke(di::Tag<di::reflect>, di::InPlaceType<MyEnum>) {
    using enum MyEnum;
    return di::make_enumerators<"MyEnum">(di::enumerator<"Foo", Foo>, di::enumerator<"Bar", Bar>,
                                          di::enumerator<"Baz", Baz>);
}

constexpr static void enum_() {
    ASSERT_EQ(di::enum_to_string(MyEnum::Foo), "Foo"_sv);
    ASSERT_EQ(di::enum_to_string(MyEnum::Bar), "Bar"_sv);
    ASSERT_EQ(di::enum_to_string(MyEnum::Baz), "Baz"_sv);
    ASSERT_EQ(di::enum_to_string(MyEnum(-1)), "[<Invalid Enum Value>]"_sv);

    ASSERT_EQ(di::to_string(MyEnum::Foo), "Foo"_sv);
    ASSERT_EQ(di::to_string(MyEnum::Bar), "Bar"_sv);
    ASSERT_EQ(di::to_string(MyEnum::Baz), "Baz"_sv);

    ASSERT_EQ(di::parse<MyEnum>("Foo"_sv), MyEnum::Foo);
    ASSERT_EQ(di::parse<MyEnum>("Bar"_sv), MyEnum::Bar);
    ASSERT_EQ(di::parse<MyEnum>("Baz"_sv), MyEnum::Baz);
    ASSERT(!di::parse<MyEnum>("Blah"_sv));
}

enum class MyFlags {
    None = 0,
    A = 1,
    B = 2,
    C = 4,
    BC = B | C,
};

DI_DEFINE_ENUM_BITWISE_OPERATIONS(MyFlags)

constexpr static auto tag_invoke(di::Tag<di::reflect>, di::InPlaceType<MyFlags>) {
    using enum MyFlags;
    return di::make_enumerators<"MyFlags">(di::enumerator<"None", None>, di::enumerator<"A", A>, di::enumerator<"B", B>,
                                           di::enumerator<"C", C>, di::enumerator<"BC", BC>);
}

constexpr static void flags() {
    ASSERT_EQ(di::enum_to_string(MyFlags::None), "None"_sv);
    ASSERT_EQ(di::enum_to_string(MyFlags::A), "A"_sv);
    ASSERT_EQ(di::enum_to_string(MyFlags::BC), "BC"_sv);
    ASSERT_EQ(di::enum_to_string(MyFlags::A | MyFlags::B | MyFlags::C), "A|BC"_sv);
    ASSERT_EQ(di::enum_to_string(MyFlags(-1)), "[<Invalid Enum Value>]"_sv);

    ASSERT_EQ(di::to_string(MyFlags::A), "A"_sv);
    ASSERT_EQ(di::to_string(MyFlags::A | MyFlags::B | MyFlags::C), "A|BC"_sv);

    ASSERT(di::valid_enum_value(MyFlags::None));
    ASSERT(di::valid_enum_value(MyFlags::A));
    ASSERT(di::valid_enum_value(MyFlags::A | MyFlags::B));
    ASSERT(!di::valid_enum_value(MyFlags(-1)));

    ASSERT_EQ(di::parse<MyFlags>("A"_sv), MyFlags::A);
    ASSERT_EQ(di::parse<MyFlags>("B"_sv), MyFlags::B);
    ASSERT_EQ(di::parse<MyFlags>("A|B"_sv), MyFlags::A | MyFlags::B);
    ASSERT_EQ(di::parse<MyFlags>("BC"_sv), MyFlags::BC);
    ASSERT(!di::parse<MyFlags>("Blah"_sv));
    ASSERT(!di::parse<MyFlags>("A|Blah"_sv));
}

TESTC(reflect, basic)
TESTC(reflect, private_fields)
TESTC(reflect, format)
TESTC(reflect, enum_)
TESTC(reflect, flags)
}
