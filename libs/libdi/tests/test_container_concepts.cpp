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
    constexpr friend di::InPlaceType<bool> tag_invoke(di::Tag<di::container::iterator_value>, di::InPlaceType<I>);
    constexpr friend di::types::ForwardIteratorTag tag_invoke(di::Tag<di::container::iterator_category>,
                                                              di::InPlaceType<I>);
    constexpr friend di::ssize_t tag_invoke(di::Tag<di::container::iterator_ssize_type>, di::InPlaceType<I>);
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
    constexpr friend di::InPlaceType<bool> tag_invoke(di::Tag<di::container::iterator_value>, di::InPlaceType<J>);
    constexpr friend di::types::InputIteratorTag tag_invoke(di::Tag<di::container::iterator_category>,
                                                            di::InPlaceType<J>);
    constexpr friend di::ssize_t tag_invoke(di::Tag<di::container::iterator_ssize_type>, di::InPlaceType<J>);
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

struct S : di::meta::EnableBorrowedContainer<S> {
    constexpr int* begin() const { return nullptr; }
    constexpr int* end() const { return nullptr; }
    constexpr size_t size() const { return 4; }
    constexpr int* data() const { return nullptr; }
};

struct F
    : di::meta::EnableView<F, false>
    , di::meta::EnableBorrowedContainer<F> {
private:
    constexpr friend bool* tag_invoke(di::Tag<di::container::begin>, F const&) { return nullptr; }
    constexpr friend bool* tag_invoke(di::Tag<di::container::end>, F const&) { return nullptr; }
    constexpr friend size_t tag_invoke(di::Tag<di::container::size>, F const&) { return 2; }
};

struct G
    : di::meta::EnableView<G>
    , di::meta::EnableBorrowedContainer<G> {
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

static_assert(di::concepts::Container<decltype(x)>);
static_assert(di::concepts::Container<S>);
static_assert(di::concepts::Container<F>);
static_assert(di::concepts::Container<G>);
static_assert(di::concepts::SizedContainer<decltype(x)>);
static_assert(di::concepts::SizedContainer<S>);
static_assert(di::concepts::SizedContainer<F>);
static_assert(di::concepts::SizedContainer<G>);
static_assert(di::concepts::ContiguousContainer<decltype(x)>);
static_assert(di::concepts::ContiguousContainer<S>);
static_assert(di::concepts::ContiguousContainer<F>);
static_assert(di::concepts::ContiguousContainer<G>);

static_assert(!di::concepts::View<decltype(x)>);
static_assert(!di::concepts::View<S>);
static_assert(!di::concepts::View<F>);
static_assert(di::concepts::View<G>);

static_assert(!di::concepts::BorrowedContainer<decltype(x)>);
static_assert(di::concepts::BorrowedContainer<S>);
static_assert(di::concepts::BorrowedContainer<F>);
static_assert(di::concepts::BorrowedContainer<G>);

static_assert(di::concepts::BorrowedContainer<decltype(x)&>);
static_assert(di::concepts::BorrowedContainer<S&>);
static_assert(di::concepts::BorrowedContainer<F&>);
static_assert(di::concepts::BorrowedContainer<G&>);
