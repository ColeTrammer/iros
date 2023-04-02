#ifdef DIUS_USE_RUNTIME
#include <ccpp/bits/all.h>
#else
#include <stdlib.h>
#endif

#include <dius/test/prelude.h>

static void malloc_free() {
    auto* a = di::black_box(malloc(42));
    ASSERT(a);

    auto* b = di::black_box(malloc(42));
    ASSERT(b);

    auto* c = di::black_box(malloc(0));
    ASSERT(c);

    ASSERT_NOT_EQ(a, b);
    ASSERT_NOT_EQ(a, c);

    free(a);
    free(b);
    free(c);
}

static void calloc_() {
    auto* result = static_cast<byte*>(di::black_box(calloc(16, 4)));
    ASSERT(result);

    ASSERT(di::container::equal(di::Span { result, result + 64 }, di::repeat(0_b, 64)));

    free(result);
}

static void aligned_alloc_() {
    // NOTE: asan claims the size passed to aligned_alloc() must be a multiple of the alignment. As per cpp reference,
    //       there is a C proposal to ignore this restriction. Glilbc appears to accept this undersized allocations, as
    //       does ccpp's implementation.
    //       https://en.cppreference.com/w/c/memory/aligned_alloc
#if !defined(DI_SANITIZER)
    constexpr auto size = 8zu;
#else
    constexpr auto size = 64zu;
#endif

    auto* a = di::black_box(aligned_alloc(16, size));
    ASSERT(a);
    ASSERT_EQ(di::to_uintptr(a) % 16, 0);
    free(a);

    auto* b = di::black_box(aligned_alloc(32, size));
    ASSERT(b);
    ASSERT_EQ(di::to_uintptr(b) % 32, 0);
    free(b);

    auto* c = di::black_box(aligned_alloc(64, size));
    ASSERT(c);
    ASSERT_EQ(di::to_uintptr(c) % 64, 0);
    free(c);
}

static void realloc_() {
    auto* a = di::black_box(realloc(nullptr, 8));
    di::container::iota(di::Span { static_cast<i8*>(a), static_cast<i8*>(a) + 8 }, 0);
    ASSERT(a);

    auto* b = di::black_box(realloc(a, 16));
    ASSERT(b);
    ASSERT(di::container::equal(di::Span { static_cast<i8*>(b), static_cast<i8*>(b) + 8 }, di::range(8)));

    auto* c = di::black_box(realloc(b, 32));
    ASSERT(c);
    ASSERT(di::container::equal(di::Span { static_cast<i8*>(c), static_cast<i8*>(c) + 8 }, di::range(8)));

    auto* d = di::black_box(realloc(c, 8));
    ASSERT(d);

    // The shrinked pointer is only guaranteed to be the same in the ccpp implementation.
#ifdef DIUS_USE_RUNTIME
    ASSERT_EQ(d, c);
#endif

    free(d);
}

TEST(stdlib_h, malloc_free)
TEST(stdlib_h, calloc_)
TEST(stdlib_h, aligned_alloc_)
TEST(stdlib_h, realloc_)
