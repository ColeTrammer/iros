#pragma once

#include <di/meta/compare.h>
#include <di/meta/core.h>
#include <di/meta/operations.h>
#include <di/vocab/md/concepts/extents.h>

namespace di::concepts {
namespace detail {
    template<typename Layout, typename Mapping>
    concept IsMappingOf = SameAs<typename Layout::template Mapping<typename Mapping::ExtentsType>, Mapping>;
}

template<typename M>
concept MDLayoutMapping = Copyable<M> && EqualityComparable<M> && requires(M const m) {
    typename M::ExtentsType;
    typename M::SizeType;
    typename M::RankType;
    typename M::LayoutType;

    { m.extents() } -> SameAs<typename M::ExtentsType const&>;
    { m.required_span_size() } -> SameAs<typename M::SizeType>;
    { m.is_unique() } -> SameAs<bool>;
    { m.is_exhaustive() } -> SameAs<bool>;
    { m.is_strided() } -> SameAs<bool>;

    { M::is_always_unique() } -> SameAs<bool>;
    { M::is_always_exhaustive() } -> SameAs<bool>;
    { M::is_always_strided() } -> SameAs<bool>;
};

template<typename T, typename Extents>
concept MDLayout = concepts::Extents<Extents> && requires { typename T::template Mapping<Extents>; } &&
                   MDLayoutMapping<typename T::template Mapping<Extents>>;
}
