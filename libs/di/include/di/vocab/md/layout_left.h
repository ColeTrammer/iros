#pragma once

#include "di/function/unpack.h"
#include "di/meta/algorithm.h"
#include "di/meta/operations.h"
#include "di/vocab/md/concepts/extents.h"

namespace di::vocab {
template<typename Extents>
class LayoutLeft::Mapping {
public:
    using ExtentsType = Extents;
    using SizeType = typename ExtentsType::SizeType;
    using RankType = typename ExtentsType::RankType;
    using LayoutType = LayoutLeft;

    Mapping() = default;
    Mapping(Mapping const&) = default;
    Mapping(Mapping&&) = default;

    constexpr Mapping(ExtentsType const& extents) : m_extents(extents) {}

    template<concepts::Extents OtherExtents>
    requires(concepts::ConstructibleFrom<ExtentsType, OtherExtents>)
    constexpr explicit(!concepts::ConvertibleTo<OtherExtents, ExtentsType>)
        Mapping(Mapping<OtherExtents> const& extents)
        : m_extents(extents) {}

    template<concepts::Extents OtherExtents>
    requires(ExtentsType::rank() <= 1 && concepts::ConstructibleFrom<ExtentsType, OtherExtents>)
    constexpr explicit(!concepts::ConvertibleTo<OtherExtents, ExtentsType>)
        Mapping(LayoutRight::Mapping<OtherExtents> const& other)
        : m_extents(other.extents()) {}

    template<concepts::Extents OtherExtents>
    requires(concepts::ConstructibleFrom<ExtentsType, OtherExtents>)
    constexpr explicit(ExtentsType::rank() > 0) Mapping(LayoutStride::Mapping<OtherExtents> const& other)
        : m_extents(other.extents()) {}

    auto operator=(Mapping const&) -> Mapping& = default;

    constexpr auto extents() const -> ExtentsType const& { return m_extents; }

    constexpr auto required_span_size() const -> SizeType { return extents().fwd_prod_of_extents(extents().rank()); }

    template<typename... Indices>
    requires(sizeof...(Indices) == ExtentsType::rank() && (concepts::ConvertibleTo<Indices, SizeType> && ...))
    constexpr auto operator()(Indices... indices) const -> SizeType {
        return function::unpack<meta::MakeIndexSequence<sizeof...(Indices)>>([&]<size_t... i>(meta::ListV<i...>) {
            return ((static_cast<SizeType>(indices) * stride(i)) + ... + 0);
        });
    }

    constexpr static auto is_always_unique() -> bool { return true; }
    constexpr static auto is_always_exhaustive() -> bool { return true; }
    constexpr static auto is_always_strided() -> bool { return true; }

    constexpr static auto is_unique() -> bool { return true; }
    constexpr static auto is_exhaustive() -> bool { return true; }
    constexpr static auto is_strided() -> bool { return true; }

    constexpr auto stride(RankType i) const -> SizeType { return extents().fwd_prod_of_extents(i); }

private:
    template<typename OtherExtents>
    requires(Extents::rank() == OtherExtents::rank())
    constexpr friend auto operator==(Mapping const& a, Mapping<OtherExtents> const& b) -> bool {
        return a.extents() == b.extents();
    }

    [[no_unique_address]] ExtentsType m_extents;
};
}
