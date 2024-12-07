#pragma once

#include "di/bit/endian/little_endian.h"
#include "di/container/action/sequence.h"
#include "di/container/concepts/input_container.h"
#include "di/container/concepts/sized_container.h"
#include "di/container/interface/size.h"
#include "di/function/monad/monad_try.h"
#include "di/io/interface/reader.h"
#include "di/io/interface/writer.h"
#include "di/io/write_exactly.h"
#include "di/meta/language.h"
#include "di/meta/operations.h"
#include "di/meta/util.h"
#include "di/serialization/serialize.h"
#include "di/types/integers.h"
#include "di/util/bit_cast.h"
#include "di/util/to_underlying.h"
#include "di/vocab/array/array.h"
#include "di/vocab/tuple/tuple_like.h"
#include "di/vocab/variant/variant_like.h"

namespace di::serialization {
struct BinaryFormat;

/// @brief A serializer for a simple binary format.
///
/// @tparam Writer The type of the writer to write to.
///
/// The binary format is a simple format. Integral and enum types are written as-is, but are converted to little endian
/// first. Structure types are serialized by calling `serialize` on each field. Tuple types are serialized by calling
/// `serialize` on each element. Variant types are serialized by writing the index of the active alternative and then
/// calling `serialize` on the active alternative. Container types are serialized by writing the size of the container
/// and then calling `serialize` on each element. Strings are just specializations of containers.
///
/// The format is not self-describing, so the deserializer must know the format of the data it is deserializing. This
/// also means that it is not possible to add new fields to a structure without breaking backwards compatibility. If
/// this is needed, the structure should be wrapped in a variant, which allows the variant index to act as the version
/// number. Including a verson number directly in a structure is not enough since the size of the structure may change.
template<Impl<io::Writer> Writer>
class BinarySerializer {
public:
    using SerializationFormat = BinaryFormat;

    template<concepts::NotDecaysTo<BinarySerializer> T>
    requires(ConstructibleFrom<Writer, T>)
    // NOLINTNEXTLINE(bugprone-forwarding-reference-overload)
    constexpr explicit BinarySerializer(T&& writer) : m_writer(di::forward<T>(writer)) {}

    template<typename T, concepts::InstanceOf<reflection::Fields> M>
    constexpr auto serialize(T&& value, M) {
        return tuple_sequence<meta::WriterResult<void, Writer>>(
            [&](auto field) {
                return di::serialize(*this, field.get(value));
            },
            M {});
    }

    constexpr auto serialize(concepts::TupleLike auto&& value) {
        return tuple_sequence<meta::WriterResult<void, Writer>>(
            [&](auto&& element) {
                return di::serialize(*this, element);
            },
            value);
    }

    constexpr auto serialize(concepts::VariantLike auto&& value) -> meta::WriterResult<void, Writer> {
        // NOTE: Explicitly casting to u64 allows forwards compatibility when adding new alternatives to a variant.
        // By default the index type would be a u8 for a variant with 256 alternatives.
        auto const index = u64(value.index());
        DI_TRY(di::serialize(*this, index));
        return visit(
            [&](auto&& alternative) {
                return di::serialize(*this, alternative);
            },
            value);
    }

    template<concepts::IntegralOrEnum T>
    constexpr auto serialize(T value) {
        auto const little_endian = LittleEndian<T>(value);
        auto const as_bytes = di::bit_cast<Array<byte, sizeof(T)>>(little_endian);
        return io::write_exactly(writer(), as_bytes);
    }

    template<typename T, concepts::InstanceOf<reflection::Atom> M>
    requires(M::is_string())
    constexpr auto serialize(T&& value, M) {
        // Serialize the underlying data, in terms of code units.
        return di::serialize(*this, value.span());
    }

    template<concepts::SizedContainer T>
    requires(concepts::Serializable<meta::ContainerReference<T>, BinarySerializer>)
    constexpr auto serialize(T&& value) -> meta::WriterResult<void, Writer> {
        // NOTE: Explicitly casting to u64 allows interoperability with 32 bit systems.
        auto const size = u64(container::size(value));
        DI_TRY(di::serialize(*this, size));
        return container::sequence(value, [&](auto&& element) {
            return di::serialize(*this, element);
        });
    }

    constexpr auto writer() & -> Writer& { return m_writer; }
    constexpr auto writer() const& -> Writer const& { return m_writer; }
    constexpr auto writer() && -> Writer&& { return util::move(*this).m_writer; }

private:
    Writer m_writer;
};

template<typename T>
BinarySerializer(T&&) -> BinarySerializer<T>;

template<Impl<io::Reader> Reader>
class BinaryDeserializer;

struct BinaryFormat {
    template<concepts::Impl<io::Writer> Writer, typename... Args>
    requires(ConstructibleFrom<BinarySerializer<meta::RemoveCVRef<Writer>>, Writer, Args...>)
    constexpr static auto serializer(Writer&& writer, Args&&... args) {
        return BinarySerializer<meta::RemoveCVRef<Writer>>(di::forward<Writer>(writer), di::forward<Args>(args)...);
    }

    template<concepts::Impl<io::Reader> Reader, typename... Args>
    requires(ConstructibleFrom<BinaryDeserializer<meta::RemoveCVRef<Reader>>, Reader, Args...>)
    constexpr static auto deserializer(Reader&& reader, Args&&... args) {
        return BinaryDeserializer<meta::RemoveCVRef<Reader>>(di::forward<Reader>(reader), di::forward<Args>(args)...);
    }
};

constexpr inline auto binary_format = BinaryFormat {};

namespace detail {
    struct SerializeBinaryFunction {
        template<Impl<io::Writer> Writer, Serializable<BinarySerializer<Writer>> T, typename... Args>
        requires(ConstructibleFrom<BinarySerializer<ReferenceWrapper<meta::RemoveReference<Writer>>>,
                                   ReferenceWrapper<meta::RemoveReference<Writer>>, Args...>)
        constexpr auto operator()(Writer&& writer, T&& value, Args&&... args) const {
            return serialize(binary_format, di::ref(writer), value, di::forward<Args>(args)...);
        }
    };
}

constexpr inline auto serialize_binary = detail::SerializeBinaryFunction {};
}

namespace di {
using serialization::binary_format;
using serialization::BinaryFormat;
using serialization::BinarySerializer;
using serialization::serialize_binary;
}
