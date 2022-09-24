#include <di/prelude.h>
#include <test/test.h>

struct Granular : di::BitFlag<32 + 23> {};

enum class IdtType : u8 { X, Y, Z };

struct Type : di::BitEnum<IdtType, 38, 4> {};

static_assert(di::concepts::BitTag<Type>);

using IdtEntry = di::BitStruct<16, Granular, Type>;

constexpr void basic() {
    auto x = IdtEntry(Granular(true), Type(IdtType::Y));
    ASSERT_EQ(x.get<Granular>(), true);
    ASSERT_EQ(x.get<Type>(), IdtType::Y);
}

TEST_CONSTEXPR(bit_bitstruct, basic, basic)