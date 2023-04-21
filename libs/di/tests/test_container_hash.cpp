#include <di/container/hash/hash_write.h>
#include <di/container/hash/prelude.h>
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

TESTC(container_hash, hash)
}
