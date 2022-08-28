#include <di/container/concepts/bidirectional_iterator.h>
#include <di/container/concepts/contiguous_iterator.h>
#include <di/container/concepts/forward_iterator.h>
#include <di/container/concepts/input_iterator.h>
#include <di/container/concepts/random_access_iterator.h>
#include <di/container/concepts/sentinel_for.h>
#include <di/container/interface/begin.h>
#include <di/container/interface/end.h>
#include <di/container/interface/size.h>
#include <di/prelude.h>

static_assert(di::concepts::InputIterator<int*>);
static_assert(di::concepts::ForwardIterator<int*>);
static_assert(di::concepts::BidirectionalIterator<int*>);
static_assert(di::concepts::RandomAccessIterator<int*>);
static_assert(di::concepts::ContiguousIterator<int*>);
static_assert(di::concepts::SentinelFor<int*, int*>);

struct I {
    constexpr bool operator*() const { return false; }
    constexpr I& operator++() { return *this; }
    constexpr I operator++(int) { return *this; }
    constexpr friend bool operator==(I, I) { return true; }

private:
    constexpr friend bool tag_invoke(di::Tag<di::container::iterator::iterator_value>, di::InPlaceType<I>);
    constexpr friend di::types::ForwardIteratorTag tag_invoke(di::Tag<di::container::iterator::iterator_category>, di::InPlaceType<I>);
};

static_assert(di::concepts::InputIterator<I>);
static_assert(di::concepts::ForwardIterator<I>);
static_assert(!di::concepts::BidirectionalIterator<I>);
static_assert(!di::concepts::RandomAccessIterator<I>);
static_assert(!di::concepts::ContiguousIterator<I>);
static_assert(di::concepts::SentinelFor<I, I>);

struct J {
    constexpr bool operator*() const { return false; }
    constexpr J& operator++() { return *this; }
    constexpr J& operator++(int) { return *this; }

private:
    constexpr friend bool tag_invoke(di::Tag<di::container::iterator::iterator_value>, di::InPlaceType<J>);
    constexpr friend di::types::InputIteratorTag tag_invoke(di::Tag<di::container::iterator::iterator_category>, di::InPlaceType<J>);
};

static_assert(di::concepts::InputIterator<J>);
static_assert(!di::concepts::ForwardIterator<J>);
static_assert(!di::concepts::BidirectionalIterator<J>);
static_assert(!di::concepts::RandomAccessIterator<J>);
static_assert(!di::concepts::ContiguousIterator<J>);
static_assert(!di::concepts::SentinelFor<J, J>);

static_assert(!di::concepts::InputIterator<int>);
static_assert(!di::concepts::ForwardIterator<int>);
static_assert(!di::concepts::BidirectionalIterator<int>);
static_assert(!di::concepts::RandomAccessIterator<int>);
static_assert(!di::concepts::ContiguousIterator<int>);
static_assert(!di::concepts::SentinelFor<int, int>);

struct S {
    constexpr int* begin() const { return nullptr; }
    constexpr int* end() const { return nullptr; }
    constexpr size_t size() const { return 4; }
};

struct F {
private:
    constexpr friend bool* tag_invoke(di::Tag<di::container::begin>, F const&) { return nullptr; }
    constexpr friend bool* tag_invoke(di::Tag<di::container::end>, F const&) { return nullptr; }
    constexpr friend size_t tag_invoke(di::Tag<di::container::size>, F const&) { return 2; }
};

struct G {
private:
    constexpr friend bool* tag_invoke(di::Tag<di::container::begin>, G const&) { return nullptr; }
    constexpr friend bool* tag_invoke(di::Tag<di::container::end>, G const&) { return nullptr; }
};

constexpr int x[5] = { 0, 1, 2, 3, 4 };
static_assert(di::container::begin(x) == x + 0);
static_assert(di::container::begin(S {}) == nullptr);
static_assert(di::container::begin(F {}) == nullptr);
static_assert(di::container::end(x) == x + 5);
static_assert(di::container::end(S {}) == nullptr);
static_assert(di::container::end(F {}) == nullptr);
static_assert(di::container::size(x) == 5);
static_assert(di::container::size(S {}) == 4);
static_assert(di::container::size(F {}) == 2);
static_assert(di::container::size(G {}) == 0);
