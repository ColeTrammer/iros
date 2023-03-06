#include <iris/core/unit_test.h>
#include <iris/mm/address_space.h>

static void basic() {
    int* x = new (std::nothrow) int;
    ASSERT(x);
    di::AtomicRef(*x).store(42, di::MemoryOrder::Relaxed);
    ASSERT_EQ(42, di::AtomicRef(*x).load(di::MemoryOrder::Relaxed));
    delete x;
}

TEST(allocation, basic)
