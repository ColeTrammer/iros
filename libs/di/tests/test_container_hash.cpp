#include <di/container/hash/hash_write.h>
#include <di/container/hash/prelude.h>
#include <di/meta/compare.h>
#include <di/vocab/tuple/prelude.h>
#include <dius/test/prelude.h>

namespace container_hash {
constexpr void hash() {
    di::hash(42);

    auto x = di::make_tuple(1, 2, 3);
    di::hash(x);

    auto y = di::Array { 1, 2, 3, 4, 5 };
    di::hash(y);

    auto z = "hello"_sv;
    di::hash(z);
}

constexpr void hash_same() {
    static_assert(di::HashSame<int, int>);
    static_assert(!di::HashSame<int, long>);

    static_assert(di::HashSame<di::String, di::StringView>);
    static_assert(!di::HashSame<di::String, di::TransparentStringView>);

    static_assert(di::HashSame<di::Tuple<di::String, int>, di::Tuple<di::StringView, int>>);
    static_assert(!di::HashSame<di::Tuple<di::String, int>, di::Tuple<di::TransparentStringView, int>>);
}

TESTC(container_hash, hash)
TESTC(container_hash, hash_same)
}
