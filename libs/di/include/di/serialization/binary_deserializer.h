#pragma once

#include <di/container/action/sequence_to.h>
#include <di/container/action/to.h>
#include <di/container/concepts/container.h>
#include <di/container/meta/container_value.h>
#include <di/container/string/string_impl_forward_declaration.h>
#include <di/container/view/range.h>
#include <di/container/view/transform.h>
#include <di/function/index_dispatch.h>
#include <di/function/monad/monad_try.h>
#include <di/io/interface/reader.h>
#include <di/io/read_exactly.h>
#include <di/meta/constexpr.h>
#include <di/meta/core.h>
#include <di/meta/language.h>
#include <di/meta/operations.h>
#include <di/platform/prelude.h>
#include <di/serialization/binary_serializer.h>
#include <di/serialization/deserialize.h>
#include <di/types/byte.h>
#include <di/types/in_place_type.h>
#include <di/types/integers.h>
#include <di/util/create.h>
#include <di/util/declval.h>
#include <di/util/reference_wrapper.h>
#include <di/vocab/array/array.h>
#include <di/vocab/expected/expected_forward_declaration.h>
#include <di/vocab/tuple/tuple_like.h>
#include <di/vocab/variant/variant_alternative.h>
#include <di/vocab/variant/variant_like.h>

namespace di::serialization {
/// @brief A deserializer for a simple binary format.
///
/// @tparam Reader The type of the reader to read from.
///
/// This deserializer is for the simple binary format used by the binary
/// serializer. It can deserialize any type that the binary serializer can
/// serialize, which includes containers, string, tuples, variants, integers,
/// and enumerators.
///
/// @see BinarySerializer
template<Impl<io::Reader> Reader>
class BinaryDeserializer {
private:
    template<typename T>
    using Result = meta::ReaderResult<T, Reader>;

public:
    using DeserializationFormat = BinaryFormat;

    template<concepts::NotDecaysTo<BinaryDeserializer> T>
    requires(ConstructibleFrom<Reader, T>)
    // NOLINTNEXTLINE(bugprone-forwarding-reference-overload)
    constexpr explicit BinaryDeserializer(T&& reader) : m_reader(di::forward<T>(reader)) {}

    template<concepts::IntegralOrEnum T>
    constexpr auto deserialize(InPlaceType<T>) -> Result<T> {
        auto bytes = Array<byte, sizeof(T)> {};
        DI_TRY(read_exactly(reader(), bytes.span()));
        auto const little_endian_value = di::bit_cast<LittleEndian<T>>(bytes);
        return T(little_endian_value);
    }

    template<concepts::TupleLike T>
    requires(concepts::DefaultConstructible<T>)
    constexpr auto deserialize(InPlaceType<T>) -> Result<T> {
        auto result = T();
        DI_TRY(tuple_sequence<Result<void>>(
            [&]<typename U>(U&& element) -> Result<void> {
                element = DI_TRY(di::deserialize<meta::RemoveCVRef<U>>(*this));
                return {};
            },
            result));
        return result;
    }

    template<concepts::VariantLike T>
    constexpr auto deserialize(InPlaceType<T>) -> Result<T> {
        using IndexType = u64;
        auto const index = DI_TRY(di::deserialize<IndexType>(*this));
        if (index >= meta::VariantSize<T>) {
            return di::Unexpected(BasicError::InvalidArgument);
        }
        return function::index_dispatch<Result<T>, meta::VariantSize<T>>(
            index, [&]<usize index>(Constexpr<index>) -> Result<T> {
                return T(c_<index>, DI_TRY(di::deserialize<meta::VariantAlternative<T, index>>(*this)));
            });
    }

    template<concepts::DefaultConstructible T, concepts::InstanceOf<reflection::Fields> M>
    constexpr auto deserialize(InPlaceType<T>, M fields) -> Result<T> {
        // NOTE: for now, this requires T be default constructible.
        auto result = T {};
        DI_TRY(tuple_sequence<Result<void>>(
            [&](auto field) -> Result<void> {
                using Value = meta::Type<decltype(field)>;
                field.get(result) = DI_TRY(di::deserialize<Value>(*this));
                return {};
            },
            fields));
        return result;
    }

    template<concepts::InstanceOf<container::string::StringImpl> Str>
    constexpr auto deserialize(InPlaceType<Str>) -> Result<Str> {
        using Vec = decltype(di::declval<Str>().take_underlying_vector());
        auto vector = DI_TRY(di::deserialize<Vec>(*this));

        // NOTE: this can be fallible because the vector may contain invalid data for the encoding.
        return util::create<Str>(di::move(vector));
    }

    template<concepts::Container T>
    requires(!concepts::InstanceOf<T, container::string::StringImpl>)
    constexpr auto deserialize(InPlaceType<T>) -> Result<T> {
        using SizeType = u64;
        auto const size = DI_TRY(di::deserialize<SizeType>(*this));
        return range(size) | transform([&](auto) {
                   return di::deserialize<meta::ContainerValue<T>>(*this);
               }) |
               container::sequence_to<T>();
    }

    constexpr auto reader() & -> Reader& { return m_reader; }
    constexpr auto reader() const& -> Reader const& { return m_reader; }
    constexpr auto reader() && -> Reader&& { return util::move(*this).m_reader; }

private:
    Reader m_reader;
};

template<typename T>
BinaryDeserializer(T&&) -> BinaryDeserializer<T>;

namespace detail {
    template<typename T>
    struct DeserializeBinaryFunction {
        template<concepts::Impl<io::Reader> Reader, typename... Args>
        requires(concepts::ConstructibleFrom<BinaryDeserializer<ReferenceWrapper<meta::RemoveCVRef<Reader>>>, Reader&,
                                             Args...> &&
                 concepts::Deserializable<T, BinaryDeserializer<ReferenceWrapper<meta::RemoveCVRef<Reader>>>>)
        constexpr auto operator()(Reader&& reader, Args&&... args) const {
            return serialization::deserialize<T>(binary_format, ref(reader), util::forward<Args>(args)...);
        }
    };
}

template<typename T>
constexpr inline auto deserialize_binary = detail::DeserializeBinaryFunction<T> {};
}

namespace di {
using serialization::BinaryDeserializer;
using serialization::deserialize_binary;
}
