#pragma once

#include "di/container/view/cartesian_product.h"
#include "di/container/view/transform.h"
#include "di/function/uncurry.h"
#include "di/meta/language.h"
#include "di/vocab/md/concepts/extents.h"
#include "di/vocab/md/concepts/md_accessor.h"
#include "di/vocab/md/concepts/md_layout.h"
#include "di/vocab/md/default_accessor.h"
#include "di/vocab/md/dextents.h"
#include "di/vocab/md/extents.h"
#include "di/vocab/md/layout_right.h"

namespace di::vocab {
template<typename Element, concepts::Extents Extents, concepts::MDLayout<Extents> Layout = LayoutRight,
         concepts::MDAccessor Accessor = DefaultAccessor<Element>>
class MDSpan {
public:
    using ExtentsType = Extents;
    using LayoutType = Layout;
    using AccessorType = Accessor;
    using MappingType = typename LayoutType::template Mapping<ExtentsType>;
    using ElementType = Element;
    using ValueType = meta::RemoveCV<ElementType>;
    using SizeType = typename ExtentsType::SizeType;
    using RankType = typename ExtentsType::RankType;
    using DataHandle = typename AccessorType::DataHandle;
    using Reference = typename AccessorType::Reference;

    constexpr static auto rank() -> RankType { return ExtentsType::rank(); }
    constexpr static auto rank_dynamic() -> RankType { return ExtentsType::rank_dynamic(); }
    constexpr static auto static_extent(RankType r) -> size_t { return ExtentsType::static_extent(r); }
    constexpr auto extent(RankType r) const -> SizeType { return extents().extent(r); }

    MDSpan()
    requires(rank_dynamic() > 0 && concepts::DefaultConstructible<DataHandle> &&
             concepts::DefaultConstructible<MappingType> && concepts::DefaultConstructible<AccessorType>)
    = default;

    MDSpan(MDSpan const&) = default;
    MDSpan(MDSpan&&) = default;

    template<typename... OtherSizeTypes>
    requires((concepts::ConvertibleTo<OtherSizeTypes, SizeType> && ...) &&
             (sizeof...(OtherSizeTypes) == rank() || ((sizeof...(OtherSizeTypes) == rank_dynamic()) &&
                                                      (concepts::ConstructibleFrom<MappingType, ExtentsType> &&
                                                       concepts::DefaultConstructible<AccessorType>) )))
    constexpr explicit MDSpan(DataHandle data_handle, OtherSizeTypes... exts)
        : m_data_handle(util::move(data_handle)), m_mapping(ExtentsType(static_cast<SizeType>(util::move(exts))...)) {}

    template<typename OtherSizeType, size_t N>
    requires(concepts::ConvertibleTo<OtherSizeType, SizeType> && (N == rank() || N == rank_dynamic()) &&
             concepts::ConstructibleFrom<MappingType, ExtentsType> && concepts::DefaultConstructible<AccessorType>)
    constexpr explicit(N != rank_dynamic()) MDSpan(DataHandle data_handle, Span<OtherSizeType, N> exts)
        : m_data_handle(util::move(data_handle)), m_mapping(ExtentsType(exts)) {}

    template<typename OtherSizeType, size_t N>
    requires(concepts::ConvertibleTo<OtherSizeType, SizeType> && (N == rank() || N == rank_dynamic()) &&
             concepts::ConstructibleFrom<MappingType, ExtentsType> && concepts::DefaultConstructible<AccessorType>)
    constexpr explicit(N != rank_dynamic()) MDSpan(DataHandle data_handle, Array<OtherSizeType, N> const& exts)
        : m_data_handle(util::move(data_handle)), m_mapping(ExtentsType(exts)) {}

    constexpr MDSpan(DataHandle data_handle, ExtentsType const& ext)
    requires(concepts::ConstructibleFrom<MappingType, ExtentsType> && concepts::DefaultConstructible<AccessorType>)
        : m_data_handle(util::move(data_handle)), m_mapping(ext) {}

    constexpr MDSpan(DataHandle data_handle, MappingType const& mapping)
    requires(concepts::DefaultConstructible<AccessorType>)
        : m_data_handle(util::move(data_handle)), m_mapping(mapping) {}

    constexpr MDSpan(DataHandle data_handle, MappingType const& mapping, AccessorType const& accessor)
        : m_data_handle(util::move(data_handle)), m_mapping(mapping), m_accessor(accessor) {}

    template<typename OtherElementType, typename OtherExtents, typename OtherLayout, typename OtherAccessor>
    requires(concepts::ConstructibleFrom<MappingType, typename OtherLayout::template Mapping<OtherExtents> const&> &&
             concepts::ConstructibleFrom<AccessorType, OtherAccessor const&>)
    explicit(!concepts::ConvertibleTo<typename OtherLayout::template Mapping<OtherExtents> const&, MappingType> ||
             !concepts::ConvertibleTo<OtherAccessor const&,
                                      AccessorType>) constexpr MDSpan(MDSpan<OtherElementType, OtherExtents,
                                                                             OtherLayout, OtherAccessor> const& other)
        : m_data_handle(other.data()), m_mapping(other.mapping()), m_accessor(other.accessor()) {}

    auto operator=(MDSpan const&) -> MDSpan& = default;
    auto operator=(MDSpan&&) -> MDSpan& = default;

    template<typename... OtherSizeTypes>
    requires(sizeof...(OtherSizeTypes) == rank() && (concepts::ConvertibleTo<OtherSizeTypes, SizeType> && ...))
    constexpr auto operator[](OtherSizeTypes... indices) const -> Reference {
        auto index = m_mapping(ExtentsType::index_cast(util::move(indices))...);
        DI_ASSERT(index < m_mapping.required_span_size());
        return m_accessor.access(m_data_handle, index);
    }

    template<typename OtherSizeType>
    requires(concepts::ConvertibleTo<OtherSizeType const&, SizeType>)
    constexpr auto operator[](Span<OtherSizeType, rank()> indices) const -> Reference {
        return function::unpack<meta::MakeIndexSequence<rank()>>([&]<size_t... i>(meta::ListV<i...>) {
            return (*this)[util::as_const(indices[i])...];
        });
    }

    template<typename OtherSizeType>
    requires(concepts::ConvertibleTo<OtherSizeType const&, SizeType>)
    constexpr auto operator[](Array<OtherSizeType, rank()> const& indices) const -> Reference {
        return (*this)[indices.span()];
    }

    template<typename... OtherSizeTypes>
    requires(sizeof...(OtherSizeTypes) == rank() && (concepts::ConvertibleTo<OtherSizeTypes, SizeType> && ...))
    constexpr auto operator()(OtherSizeTypes... indices) const -> Reference {
        auto index = m_mapping(ExtentsType::index_cast(util::move(indices))...);
        DI_ASSERT(index < m_mapping.required_span_size());
        return m_accessor.access(m_data_handle, index);
    }

    template<typename OtherSizeType>
    requires(concepts::ConvertibleTo<OtherSizeType const&, SizeType>)
    constexpr auto operator()(Span<OtherSizeType, rank()> indices) const -> Reference {
        return function::unpack<meta::MakeIndexSequence<rank()>>([&]<size_t... i>(meta::ListV<i...>) {
            return (*this)[util::as_const(indices[i])...];
        });
    }

    template<typename OtherSizeType>
    requires(concepts::ConvertibleTo<OtherSizeType const&, SizeType>)
    constexpr auto operator()(Array<OtherSizeType, rank()> const& indices) const -> Reference {
        return (*this)[indices.span()];
    }

    constexpr auto each() const {
        return function::unpack<meta::MakeIndexSequence<rank()>>(
            [&]<usize... rank_indices>(meta::ListV<rank_indices...>) {
                return container::view::cartesian_product(container::view::range(extent(rank_indices))...) |
                       container::view::transform(function::uncurry([&](auto... indices) -> Reference {
                           return (*this)(indices...);
                       }));
            });
    }

    constexpr auto size() const -> size_t { return extents().fwd_prod_of_extents(rank()); }
    constexpr auto empty() const -> bool { return size() == 0; }

    constexpr auto extents() const -> ExtentsType const& { return m_mapping.extents(); }
    constexpr auto data() const -> DataHandle const& { return m_data_handle; }
    constexpr auto mapping() const -> MappingType const& { return m_mapping; }
    constexpr auto accessor() const -> AccessorType const& { return m_accessor; }

    constexpr static auto is_always_unique() -> bool { return MappingType::is_always_unique(); }
    constexpr static auto is_always_contiguous() -> bool { return MappingType::is_always_contiguous(); }
    constexpr static auto is_always_strided() -> bool { return MappingType::is_always_strided(); }

    constexpr auto is_unique() const -> bool { return m_mapping.is_unique(); }
    constexpr auto is_contiguous() const -> bool { return m_mapping.is_contiguous(); }
    constexpr auto is_strided() const -> bool { return m_mapping.is_strided(); }
    constexpr auto stride(RankType r) const -> SizeType { return m_mapping.stride(r); }

private:
    [[no_unique_address]] DataHandle m_data_handle {};
    [[no_unique_address]] MappingType m_mapping {};
    [[no_unique_address]] AccessorType m_accessor {};
};

template<concepts::LanguageArray CArray>
requires(meta::ArrayRank<CArray> == 1)
MDSpan(CArray&) -> MDSpan<meta::RemoveAllExtents<CArray>, Extents<size_t, meta::Extent<CArray, 0>>>;

template<typename Pointer>
requires(concepts::Pointer<meta::RemoveReference<Pointer>>)
MDSpan(Pointer&&) -> MDSpan<meta::RemovePointer<meta::RemoveReference<Pointer>>, Extents<size_t>>;

template<typename ElementType, typename... integrals>
requires(sizeof...(integrals) > 0 && (concepts::ConvertibleTo<integrals, size_t> && ...))
explicit MDSpan(ElementType*, integrals...) -> MDSpan<ElementType, Dextents<size_t, sizeof...(integrals)>>;

template<typename ElementType, typename OtherSizeType, size_t N>
MDSpan(ElementType*, Span<OtherSizeType, N>) -> MDSpan<ElementType, Dextents<size_t, N>>;

template<typename ElementType, typename OtherSizeType, size_t N>
MDSpan(ElementType*, Array<OtherSizeType, N> const&) -> MDSpan<ElementType, Dextents<size_t, N>>;

template<typename ElementType, typename SizeType, size_t... extents>
MDSpan(ElementType*, Extents<SizeType, extents...> const&) -> MDSpan<ElementType, Extents<SizeType, extents...>>;

template<class ElementType, class MappingType>
MDSpan(ElementType*, MappingType const&)
    -> MDSpan<ElementType, typename MappingType::ExtentsType, typename MappingType::LayoutType>;

template<class MappingType, class AccessorType>
MDSpan(typename AccessorType::DataHandle const&, MappingType const&, AccessorType const&)
    -> MDSpan<typename AccessorType::ElementType, typename MappingType::ExtentsType, typename MappingType::LayoutType,
              AccessorType>;
}

namespace di {
using vocab::MDSpan;
}
