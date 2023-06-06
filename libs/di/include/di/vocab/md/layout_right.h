#pragma once

#include <di/concepts/conjunction.h>
#include <di/concepts/convertible_to.h>
#include <di/function/unpack.h>
#include <di/meta/make_index_sequence.h>
#include <di/vocab/md/concepts/extents.h>

namespace di::vocab {
template<typename Extents>
class LayoutRight::Mapping {
public:
    using ExtentsType = Extents;
    using SizeType = typename ExtentsType::SizeType;
    using RankType = typename ExtentsType::RankType;
    using LayoutType = LayoutRight;

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
        Mapping(LayoutLeft::Mapping<OtherExtents> const& other)
        : m_extents(other.extents()) {}

    template<concepts::Extents OtherExtents>
    requires(concepts::ConstructibleFrom<ExtentsType, OtherExtents>)
    constexpr explicit(ExtentsType::rank() > 0) Mapping(LayoutStride::Mapping<OtherExtents> const& other)
        : m_extents(other.extents()) {}

    Mapping& operator=(Mapping const&) = default;

    constexpr ExtentsType const& extents() const { return m_extents; }

    constexpr SizeType required_span_size() const { return extents().fwd_prod_of_extents(extents().rank()); }

    template<typename... Indices>
    requires(sizeof...(Indices) == ExtentsType::rank() &&
             concepts::Conjunction<concepts::ConvertibleTo<Indices, SizeType>...>)
    constexpr SizeType operator()(Indices... indices) const {
        return function::unpack<meta::MakeIndexSequence<sizeof...(Indices)>>([&]<size_t... i>(meta::ListV<i...>) {
            return ((static_cast<SizeType>(indices) * stride(i)) + ... + 0);
        });
    }

    constexpr static bool is_always_unique() { return true; }
    constexpr static bool is_always_exhaustive() { return true; }
    constexpr static bool is_always_strided() { return true; }

    constexpr static bool is_unique() { return true; }
    constexpr static bool is_exhaustive() { return true; }
    constexpr static bool is_strided() { return true; }

    constexpr SizeType stride(RankType i) const { return extents().rev_prod_of_extents(i); }

private:
    template<typename OtherExtents>
    requires(Extents::rank() == OtherExtents::rank())
    constexpr friend bool operator==(Mapping const& a, Mapping<OtherExtents> const& b) {
        return a.extents() == b.extents();
    }

    [[no_unique_address]] ExtentsType m_extents;
};
}
