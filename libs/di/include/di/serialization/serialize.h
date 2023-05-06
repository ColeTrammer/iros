#pragma once

#include <di/concepts/class.h>
#include <di/function/tag_invoke.h>
#include <di/io/interface/writer.h>
#include <di/reflect/reflect.h>
#include <di/types/in_place_type.h>

namespace di::concepts {
template<typename T>
concept Serializer = requires(T& serializer) {
    typename meta::RemoveCVRef<T>::SerializationFormat;
    { serializer.writer() } -> Impl<io::Writer>;
    util::as_const(serializer).writer();
    util::move(serializer).writer();
};
}

namespace di::meta {
template<concepts::Serializer S>
using SerializationFormat = typename meta::RemoveCVRef<S>::SerializationFormat;
}

namespace di::serialization {
namespace detail {
    struct SerializerFunction {
        template<typename Format, concepts::Impl<io::Writer> Writer, typename... Args>
        requires(concepts::TagInvocable<SerializerFunction, Format, Writer, Args...> ||
                 requires { Format::serializer(util::declval<Writer>(), util::declval<Args>()...); })
        constexpr concepts::Serializer auto operator()(Format, Writer&& writer, Args&&... args) const {
            if constexpr (concepts::TagInvocable<SerializerFunction, Format, Writer, Args...>) {
                return function::tag_invoke(*this, Format {}, writer, util::forward<Args>(args)...);
            } else {
                return Format::serializer(util::forward<Writer>(writer), util::forward<Args>(args)...);
            }
        }
    };
}

constexpr inline auto serializer = detail::SerializerFunction {};
}

namespace di::concepts {
template<typename T, typename Writer = any::AnyRef<io::Writer>, typename... Args>
concept SerializationFormat = requires(T format, Writer&& writer, Args&&... args) {
    serialization::serializer(format, util::forward<Writer>(writer), util::forward<Args>(args)...);
};
}

namespace di::meta {
template<typename T, typename Writer = any::AnyRef<io::Writer>, typename... Args>
requires(concepts::SerializationFormat<T, Writer, Args...>)
using Serializer =
    decltype(serialization::serializer(util::declval<T>(), util::declval<Writer>(), util::declval<Args>()...));
}

namespace di::serialization {
namespace detail {
    struct SerializeMetadataFunction {
        template<typename T, typename S, typename U = meta::RemoveCVRef<T>,
                 concepts::SerializationFormat V = meta::RemoveCVRef<S>>
        requires(concepts::TagInvocable<SerializeMetadataFunction, InPlaceType<U>, InPlaceType<V>> ||
                 concepts::TagInvocable<SerializeMetadataFunction, InPlaceType<U>> || concepts::Reflectable<T>)
        constexpr concepts::ReflectionValue auto operator()(InPlaceType<T>, InPlaceType<S>) const {
            if constexpr (concepts::TagInvocable<SerializeMetadataFunction, InPlaceType<U>, InPlaceType<V>>) {
                return function::tag_invoke(*this, in_place_type<U>, in_place_type<V>);
            } else if constexpr (concepts::TagInvocable<SerializeMetadataFunction, InPlaceType<U>>) {
                return function::tag_invoke(*this, in_place_type<U>);
            } else {
                return reflection::reflect(in_place_type<U>);
            }
        }
    };
}

constexpr inline auto serialize_metadata = detail::SerializeMetadataFunction {};
}

namespace di::meta {
template<concepts::SerializationFormat S, typename T>
using SerializeMetadata = decltype(serialization::serialize_metadata(in_place_type<T>, in_place_type<S>));
}

namespace di::serialization {
namespace detail {
    struct SerializeFunction {
        template<concepts::Serializer S, typename T, typename F = meta::SerializationFormat<S>>
        requires(concepts::TagInvocable<SerializeFunction, F, S&, T&>)
        constexpr auto operator()(S&& serializer, T&& value) const {
            return function::tag_invoke(*this, F(), serializer, value);
        }

        template<concepts::Serializer S, typename T,
                 typename M = meta::SerializeMetadata<meta::SerializationFormat<S>, T>>
        requires(!concepts::TagInvocable<SerializeFunction, S&, T&> &&
                 (concepts::TagInvocable<SerializeFunction, S&, T&, M> ||
                  requires(S& serializer, T& value) { serializer.serialize(value, M()); }))
        constexpr auto operator()(S&& serializer, T&& value) const {
            if constexpr (concepts::TagInvocable<SerializeFunction, S&, T&, M>) {
                return function::tag_invoke(*this, serializer, value, M());
            } else {
                return serializer.serialize(value, M());
            }
        }

        template<typename Format, concepts::Impl<io::Writer> Writer, typename T, typename... Args>
        requires(concepts::SerializationFormat<Format, Writer, Args...>)
        constexpr void operator()(Format format, Writer&& writer, T&& value, Args&&... args) const
        requires(requires {
            (*this)(serialization::serializer(format, util::forward<Writer>(writer), util::forward<Args>(args)...),
                    value);
        })
        {
            return (*this)(
                serialization::serializer(format, util::forward<Writer>(writer), util::forward<Args>(args)...), value);
        }
    };
}

constexpr inline auto serialize = detail::SerializeFunction {};
}

namespace di::concepts {
template<typename T, typename S>
concept Serializable =
    concepts::Serializer<S> && requires(S&& serializer, T&& value) { serialization::serialize(serializer, value); };
}

namespace di::meta {
template<typename S, concepts::Serializable<S> T>
using SerializeResult = decltype(serialization::serialize(util::declval<S>(), util::declval<T>()));
}

namespace di {
using concepts::Serializable;
using concepts::SerializationFormat;
using concepts::Serializer;

using meta::SerializeMetadata;
using meta::SerializeResult;

using serialization::serialize;
using serialization::serializer;
}
