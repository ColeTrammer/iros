#pragma once

#include <di/container/view/single.h>
#include <di/container/view/transform.h>
#include <di/container/view/view.h>
#include <di/function/tag_invoke.h>
#include <di/meta/core.h>
#include <di/meta/language.h>
#include <di/meta/operations.h>
#include <di/types/prelude.h>
#include <di/vocab/span/prelude.h>
#include <di/vocab/tuple/prelude.h>

namespace di::meta {
template<typename T>
using EncodingCodeUnit = RemoveCVRef<T>::CodeUnit;

template<typename T>
using EncodingCodePoint = RemoveCVRef<T>::CodePoint;

template<typename T>
using EncodingIterator = RemoveCVRef<T>::Iterator;
}

namespace di::container::string::encoding {
namespace detail {
    struct UniversalFunction {
        template<typename T>
        constexpr bool operator()(InPlaceType<T>) const {
            if constexpr (concepts::TagInvocable<UniversalFunction, InPlaceType<T>>) {
                return function::tag_invoke(*this, in_place_type<T>);
            } else {
                return false;
            }
        }
    };

    struct ContiguousFunction {
        template<typename T>
        constexpr bool operator()(InPlaceType<T>) const {
            if constexpr (concepts::TagInvocable<ContiguousFunction, InPlaceType<T>>) {
                return function::tag_invoke(*this, in_place_type<T>);
            } else {
                return false;
            }
        }
    };

    struct NullTerminatedFunction {
        template<typename T>
        constexpr bool operator()(InPlaceType<T>) const {
            if constexpr (concepts::TagInvocable<NullTerminatedFunction, InPlaceType<T>>) {
                return function::tag_invoke(*this, in_place_type<T>);
            } else {
                return false;
            }
        }
    };
}

constexpr inline auto universal = detail::UniversalFunction {};
constexpr inline auto contiguous = detail::ContiguousFunction {};
constexpr inline auto null_terminated = detail::NullTerminatedFunction {};

template<typename T>
concept Universal = universal(in_place_type<meta::RemoveCVRef<T>>);

template<typename T>
concept Contiguous = contiguous(in_place_type<meta::RemoveCVRef<T>>);

template<typename T>
concept NullTerminated = null_terminated(in_place_type<meta::RemoveCVRef<T>>);

namespace detail {
    struct ValidateFunction {
        template<typename T>
        requires(Universal<T> ||
                 concepts::TagInvocableTo<ValidateFunction, bool, T const&, Span<meta::EncodingCodeUnit<T> const>>)
        constexpr bool operator()(T const& encoding, Span<meta::EncodingCodeUnit<T> const> code_units) const {
            if constexpr (universal(in_place_type<T>)) {
                return true;
            } else {
                return function::tag_invoke(*this, encoding, code_units);
            }
        }
    };

    struct ValidByteOffsetFunction {
        template<typename T, typename U = meta::EncodingCodeUnit<T>>
        requires(Contiguous<T> ||
                 concepts::TagInvocableTo<ValidByteOffsetFunction, bool, T const&, Span<U const>, size_t>)
        constexpr bool operator()(T const& encoding, Span<U const> code_units, size_t offset) const {
            if constexpr (Contiguous<T>) {
                return offset <= code_units.size();
            } else {
                return function::tag_invoke(*this, encoding, code_units, offset);
            }
        }
    };

    struct MakeIteratorFunction {
        template<typename T, typename U = meta::EncodingCodeUnit<T>, typename Iter = meta::EncodingIterator<T>>
        requires(concepts::TagInvocableTo<MakeIteratorFunction, Iter, T const&, Span<U const>, size_t> ||
                 concepts::ConstructibleFrom<Iter, U const*>)
        constexpr Iter operator()(T const& encoding, Span<U const> code_units, size_t offset) const {
            if constexpr (concepts::TagInvocableTo<MakeIteratorFunction, Iter, T const&, Span<U const>, size_t>) {
                return function::tag_invoke(*this, encoding, code_units, offset);
            } else {
                return Iter(code_units.data() + offset);
            }
        }
    };

    struct IteratorDataFunction {
        template<typename T, typename U = meta::EncodingCodeUnit<T>, typename Iter = meta::EncodingIterator<T>>
        requires(concepts::ExplicitlyConvertibleTo<Iter, U const*>)
        constexpr U* operator()(T const&, Span<U>, Iter iterator) const {
            // NOTE: this is safe since we have a "mutable" storage to the underlying code units.
            return const_cast<U*>(static_cast<U const*>(iterator));
        }
    };

    struct ConvertToCodeUnitsFunction {
        template<typename T, typename U = meta::EncodingCodeUnit<T>, typename P = meta::EncodingCodePoint<T>>
        requires(concepts::TagInvocable<ConvertToCodeUnitsFunction, T const&, P> || concepts::SameAs<U, P>)
        constexpr concepts::ContainerOf<U> auto operator()(T const& encoding, P code_point) const {
            if constexpr (concepts::TagInvocable<ConvertToCodeUnitsFunction, T const&, P>) {
                return function::tag_invoke(*this, encoding, code_point);
            } else {
                return view::single(code_point);
            }
        }
    };
}
constexpr inline auto validate = detail::ValidateFunction {};
constexpr inline auto valid_byte_offset = detail::ValidByteOffsetFunction {};
constexpr inline auto make_iterator = detail::MakeIteratorFunction {};
constexpr inline auto iterator_data = detail::IteratorDataFunction {};
constexpr inline auto convert_to_code_units = detail::ConvertToCodeUnitsFunction {};

namespace detail {
    template<typename V, typename P>
    concept CodePointView = concepts::ContainerOf<V, P> && concepts::View<V> && concepts::BidirectionalContainer<V> &&
                            concepts::TupleLike<V> && (meta::TupleSize<V> == 2);

    struct CodePointViewFunction {
        template<typename T, typename U = meta::EncodingCodeUnit<T>, typename P = meta::EncodingCodePoint<T>>
        constexpr CodePointView<P> auto operator()(T const& encoding, Span<U const> code_units) const {
            if constexpr (concepts::TagInvocable<CodePointViewFunction, T const&, Span<U const>>) {
                return function::tag_invoke(*this, encoding, code_units);
            } else {
                return container::View(make_iterator(encoding, code_units, 0),
                                       make_iterator(encoding, code_units, code_units.size()));
            }
        }
    };
}

constexpr inline auto code_point_view = detail::CodePointViewFunction {};

namespace detail {
    template<typename V>
    concept UnicodeCodePointView = concepts::ContainerOf<V, c32> && concepts::View<V> && concepts::ForwardContainer<V>;

    struct UnicodeCodePointViewFunction {
        template<typename T, typename U = meta::EncodingCodePoint<T>, typename P = meta::EncodingCodePoint<T>>
        requires(concepts::TagInvocable<UnicodeCodePointViewFunction, T const&, Span<U const>> ||
                 concepts::SameAs<P, c32> || concepts::ConstructibleFrom<c32, P>)
        constexpr UnicodeCodePointView auto operator()(T const& encoding, Span<U const> code_units) const {
            if constexpr (concepts::TagInvocable<UnicodeCodePointViewFunction, T const&, Span<U const>>) {
                return function::tag_invoke(*this, encoding, code_units);
            } else if constexpr (concepts::SameAs<P, c32>) {
                return code_point_view(encoding, code_units);
            } else {
                return code_point_view(encoding, code_units) | view::transform([](auto code_point) {
                           return c32(code_point);
                       });
            }
        }
    };

    struct UnicodeCodePointUnwrapFunction {
        template<typename T, typename Input, typename U = meta::EncodingCodePoint<T>,
                 typename P = meta::EncodingCodePoint<T>>
        requires(concepts::TagInvocable<UnicodeCodePointUnwrapFunction, T const&, Input> || concepts::SameAs<P, c32> ||
                 concepts::ConstructibleFrom<c32, P>)
        constexpr meta::EncodingIterator<T> operator()(T const& encoding, Input it) const {
            if constexpr (concepts::TagInvocable<UnicodeCodePointViewFunction, T const&, Input>) {
                return function::tag_invoke(*this, encoding, util::move(it));
            } else if constexpr (concepts::SameAs<P, c32>) {
                return it;
            } else {
                return it.base();
            }
        }
    };
}

constexpr inline auto unicode_code_point_view = detail::UnicodeCodePointViewFunction {};
constexpr inline auto unicode_code_point_unwrap = detail::UnicodeCodePointUnwrapFunction {};
}

namespace di::concepts {
template<typename T>
concept Encoding = concepts::Semiregular<T> &&
                   requires {
                       typename meta::EncodingCodeUnit<T>;
                       typename meta::EncodingCodePoint<T>;
                       typename meta::EncodingIterator<T>;
                   } &&
                   requires(T const& encoding, size_t offset, vocab::Span<meta::EncodingCodeUnit<T> const> code_units,
                            vocab::Span<meta::EncodingCodeUnit<T>> mutable_code_units,
                            meta::EncodingCodePoint<T> code_point, meta::EncodingIterator<T> iterator) {
                       container::string::encoding::validate(encoding, code_units);
                       container::string::encoding::valid_byte_offset(encoding, code_units, offset);
                       container::string::encoding::make_iterator(encoding, code_units, offset);
                       container::string::encoding::iterator_data(encoding, mutable_code_units, iterator);
                       container::string::encoding::convert_to_code_units(encoding, code_point);
                       container::string::encoding::code_point_view(encoding, code_units);
                       container::string::encoding::unicode_code_point_view(encoding, code_units);
                       container::string::encoding::unicode_code_point_unwrap(encoding, iterator);
                   };

template<typename T>
concept HasEncoding = requires { typename meta::RemoveCVRef<T>::Encoding; };
}

namespace di::meta {
template<concepts::HasEncoding T>
using Encoding = meta::RemoveCVRef<T>::Encoding;
}

namespace di::container::string::encoding {
struct AssumeValid {};

constexpr inline auto assume_valid = AssumeValid {};
}

namespace di {
namespace encoding = container::string::encoding;
}
