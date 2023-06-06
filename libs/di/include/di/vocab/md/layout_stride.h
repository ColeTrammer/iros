#pragma once

#include <di/concepts/conjunction.h>
#include <di/concepts/convertible_to.h>
#include <di/container/algorithm/copy.h>
#include <di/container/view/range.h>
#include <di/function/unpack.h>
#include <di/meta/constexpr.h>
#include <di/meta/index_sequence.h>
#include <di/meta/make_index_sequence.h>
#include <di/vocab/array/prelude.h>
#include <di/vocab/md/concepts/extents.h>
#include <di/vocab/span/prelude.h>

namespace di::vocab {
namespace detail {
    template<class Layout, class Mapping>
    concept IsMappingOf = concepts::SameAs<typename Layout::template Mapping<typename Mapping::ExtentsType>, Mapping>;

    template<typename M>
    concept LayoutMappingAlike = requires {
        requires concepts::Extents<typename M::ExtentsType>;
        { M::is_always_strided() } -> concepts::SameAs<bool>;
        { M::is_always_exhaustive() } -> concepts::SameAs<bool>;
        { M::is_always_unique() } -> concepts::SameAs<bool>;
        Constexpr<M::is_always_strided()>::value;
        Constexpr<M::is_always_exhaustive()>::value;
        Constexpr<M::is_always_unique()>::value;
    };
}

template<typename Extents>
class LayoutStride::Mapping {
public:
    using ExtentsType = Extents;
    using SizeType = typename ExtentsType::SizeType;
    using RankType = typename ExtentsType::RankType;
    using LayoutType = LayoutStride;

private:
    constexpr static RankType rank = ExtentsType::rank();

public:
    Mapping() = default;
    Mapping(Mapping const&) = default;
    Mapping(Mapping&&) = default;

    constexpr Mapping(ExtentsType const& extents) : m_extents(extents) {}

    template<typename OtherSizeType>
    requires(concepts::ConvertibleTo<OtherSizeType const&, SizeType> &&
             concepts::ConstructibleFrom<SizeType, OtherSizeType const&>)
    constexpr Mapping(ExtentsType const& extents, Span<OtherSizeType, rank> strides) : m_extents(extents) {
        container::copy(strides, m_strides.data());
    }

    template<typename OtherSizeType>
    requires(concepts::ConvertibleTo<OtherSizeType const&, SizeType> &&
             concepts::ConstructibleFrom<SizeType, OtherSizeType const&>)
    constexpr Mapping(ExtentsType const& extents, Array<OtherSizeType, rank> const& strides) : m_extents(extents) {
        container::copy(strides, m_strides.data());
    }

    template<detail::LayoutMappingAlike StridedLayoutMapping>
    requires(concepts::ConstructibleFrom<ExtentsType, typename StridedLayoutMapping::ExtentsType> &&
             StridedLayoutMapping::is_always_unique() && StridedLayoutMapping::is_always_strided())
    constexpr explicit((!concepts::ConvertibleTo<typename StridedLayoutMapping::ExtentsType, ExtentsType>) &&(
        detail::IsMappingOf<LayoutLeft, StridedLayoutMapping> ||
        detail::IsMappingOf<LayoutRight, StridedLayoutMapping> ||
        detail::IsMappingOf<LayoutStride, StridedLayoutMapping>) ) Mapping(StridedLayoutMapping const& other)
        : m_extents(other.extents()) {
        for (auto d : container::view::range(rank)) {
            m_strides[d] = other.stride(d);
        }
    }

    Mapping& operator=(Mapping const&) = default;

    constexpr ExtentsType const& extents() const { return m_extents; }
    constexpr Array<SizeType, rank> strides() const { return m_strides; }

    constexpr SizeType required_span_size() const {
        SizeType size = 1;
        for (auto d : container::view::range(rank)) {
            if (extents().extent(0) == 0) {
                return 0;
            }
            size += (static_cast<SizeType>(extents().extent(d) - 1)) * stride(d);
        }
        return size;
    }

    template<typename... Indices>
    requires(sizeof...(Indices) == ExtentsType::rank() &&
             concepts::Conjunction<concepts::ConvertibleTo<Indices, SizeType>...>)
    constexpr SizeType operator()(Indices... indices) const {
        return function::unpack<meta::MakeIndexSequence<sizeof...(Indices)>>(
            [&]<size_t... i>(meta::IndexSequence<i...>) {
                return ((static_cast<SizeType>(indices) * stride(i)) + ... + 0);
            });
    }

    constexpr static bool is_always_unique() { return true; }
    constexpr static bool is_always_exhaustive() { return false; }
    constexpr static bool is_always_strided() { return true; }

    constexpr static bool is_unique() { return true; }
    constexpr bool is_exhaustive() const { return required_span_size() == extents().fwd_prod_of_extents(rank); }
    constexpr static bool is_strided() { return true; }

    constexpr SizeType stride(RankType i) const { return m_strides[i]; }

private:
    template<typename OtherExtents>
    requires(Extents::rank() == OtherExtents::rank())
    constexpr friend bool operator==(Mapping const& a, Mapping<OtherExtents> const& b) {
        return a.extents() == b.extents() && a.m_strides == b.m_strides;
    }

    [[no_unique_address]] ExtentsType m_extents {};
    [[no_unique_address]] Array<SizeType, rank> m_strides {};
};
}
