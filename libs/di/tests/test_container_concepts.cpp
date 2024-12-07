#include "di/container/concepts/prelude.h"
#include "di/function/prelude.h"

namespace container_concepts {
static_assert(di::concepts::InputIterator<int*>);
static_assert(di::concepts::ForwardIterator<int*>);
static_assert(di::concepts::BidirectionalIterator<int*>);
static_assert(di::concepts::RandomAccessIterator<int*>);
static_assert(di::concepts::ContiguousIterator<int*>);
static_assert(di::concepts::SentinelFor<int*, int*>);

struct I {
    constexpr auto operator*() const -> bool { return false; }
    constexpr auto operator++() -> I& { return *this; }
    constexpr auto operator++(int) -> I { return *this; }
    constexpr friend auto operator==(I, I) -> bool { return true; }

private:
    constexpr friend auto tag_invoke(di::Tag<di::container::iterator_value>, di::InPlaceType<I>)
        -> di::InPlaceType<bool>;
    constexpr friend auto tag_invoke(di::Tag<di::container::iterator_category>, di::InPlaceType<I>)
        -> di::types::ForwardIteratorTag;
    constexpr friend auto tag_invoke(di::Tag<di::container::iterator_ssize_type>, di::InPlaceType<I>) -> di::ssize_t;
};

static_assert(di::concepts::InputIterator<I>);
static_assert(di::concepts::ForwardIterator<I>);
static_assert(!di::concepts::BidirectionalIterator<I>);
static_assert(!di::concepts::RandomAccessIterator<I>);
static_assert(!di::concepts::ContiguousIterator<I>);
static_assert(di::concepts::SentinelFor<I, I>);

struct J {
    constexpr auto operator*() const -> bool { return false; }
    constexpr auto operator++() -> J& { return *this; }
    constexpr auto operator++(int) -> J& { return *this; }

private:
    constexpr friend auto tag_invoke(di::Tag<di::container::iterator_value>, di::InPlaceType<J>)
        -> di::InPlaceType<bool>;
    constexpr friend auto tag_invoke(di::Tag<di::container::iterator_category>, di::InPlaceType<J>)
        -> di::types::InputIteratorTag;
    constexpr friend auto tag_invoke(di::Tag<di::container::iterator_ssize_type>, di::InPlaceType<J>) -> di::ssize_t;
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
    constexpr auto begin() const -> int* { return nullptr; }
    constexpr auto end() const -> int* { return nullptr; }
    constexpr auto size() const -> size_t { return 4; }
    constexpr auto data() const -> int* { return nullptr; }
};

struct F
    : di::meta::EnableView<F, false>
    , di::meta::EnableBorrowedContainer<F> {
private:
    constexpr friend auto tag_invoke(di::Tag<di::container::begin>, F const&) -> bool* { return nullptr; }
    constexpr friend auto tag_invoke(di::Tag<di::container::end>, F const&) -> bool* { return nullptr; }
    constexpr friend auto tag_invoke(di::Tag<di::container::size>, F const&) -> size_t { return 2; }
};

struct G
    : di::meta::EnableView<G>
    , di::meta::EnableBorrowedContainer<G> {
private:
    constexpr friend auto tag_invoke(di::Tag<di::container::begin>, G const&) -> bool* { return nullptr; }
    constexpr friend auto tag_invoke(di::Tag<di::container::end>, G const&) -> bool* { return nullptr; }
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
}
