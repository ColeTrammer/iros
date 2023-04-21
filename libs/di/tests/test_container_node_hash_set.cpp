#include <di/container/hash/node/prelude.h>
#include <dius/test/prelude.h>

namespace container_node_hash_set {
constexpr void basic() {
    auto x = di::NodeHashSet<int> {};
    x.reserve(10);

    // x.insert(1);
    // x.insert(2);
    // x.insert(3);
    // x.insert(4);
    // x.insert(5);
}

TESTC(container_node_hash_set, basic)
}
