#pragma once

#include <di/function/tag_invoke.h>
#include <di/io/interface/reader.h>
#include <di/meta/core.h>
#include <di/reflect/reflect.h>
#include <di/serialization/serialize.h>

namespace di::concepts {
template<typename T>
concept Deserializer = requires(T& serializer) {
    typename meta::RemoveCVRef<T>::DeserializationFormat;
    { serializer.reader() } -> Impl<io::Reader>;
    util::as_const(serializer).reader();
    util::move(serializer).reader();
};
}

namespace di::meta {
template<concepts::Deserializer S>
using DeserializationFormat = typename meta::RemoveCVRef<S>::DeserializationFormat;
}

namespace di::serialization {
namespace detail {
    struct DeserializerFunction {
        template<typename Format, concepts::Impl<io::Reader> Reader, typename... Args>
        requires(concepts::TagInvocable<DeserializerFunction, Format, Reader, Args...> ||
                 requires { Format::deserializer(util::declval<Reader>(), util::declval<Args>()...); })
        constexpr auto operator()(Format, Reader&& reader, Args&&... args) const -> concepts::Deserializer auto {
            if constexpr (concepts::TagInvocable<DeserializerFunction, Format, Reader, Args...>) {
                return function::tag_invoke(*this, Format {}, util::forward<Reader>(reader),
                                            util::forward<Args>(args)...);
            } else {
                return Format::deserializer(util::forward<Reader>(reader), util::forward<Args>(args)...);
            }
        }
    };
}

constexpr inline auto deserializer = detail::DeserializerFunction {};
}

namespace di::concepts {
template<typename T, typename Reader = any::AnyRef<io::Reader>, typename... Args>
concept DeserializationFormat = requires(T format, Reader&& reader, Args&&... args) {
    serialization::deserializer(format, util::forward<Reader>(reader), util::forward<Args>(args)...);
};
}

namespace di::meta {
template<typename T, typename Reader = any::AnyRef<io::Reader>, typename... Args>
requires(concepts::DeserializationFormat<T, Reader, Args...>)
using Deserializer =
    decltype(serialization::deserializer(util::declval<T>(), util::declval<Reader>(), util::declval<Args>()...));

template<typename S, typename T>
using DeserializeResult = meta::LikeExpected<meta::ReaderResult<void, decltype(util::declval<S>().reader())>, T>;
}

namespace di::serialization {
namespace detail {
    struct DeserializeMetadataFunction {
        template<typename T, typename F, typename U = meta::RemoveCVRef<T>,
                 concepts::DeserializationFormat V = meta::RemoveCVRef<F>>
        requires(concepts::TagInvocable<DeserializeMetadataFunction, InPlaceType<U>, InPlaceType<V>> ||
                 concepts::TagInvocable<DeserializeMetadataFunction, InPlaceType<U>> ||
                 requires { typename meta::SerializeMetadata<V, T>; } || concepts::Reflectable<T>)
        constexpr auto operator()(InPlaceType<T>, InPlaceType<F>) const -> concepts::ReflectionValue auto {
            if constexpr (concepts::TagInvocable<DeserializeMetadataFunction, InPlaceType<U>, InPlaceType<V>>) {
                return function::tag_invoke(*this, in_place_type<U>, in_place_type<V>);
            } else if constexpr (concepts::TagInvocable<DeserializeMetadataFunction, InPlaceType<U>>) {
                return function::tag_invoke(*this, in_place_type<U>);
            } else if constexpr (requires { typename meta::SerializeMetadata<V, T>; }) {
                return meta::SerializeMetadata<V, T> {};
            } else {
                return reflection::reflect(in_place_type<U>);
            }
        }
    };
}

constexpr inline auto deserialize_metadata = detail::DeserializeMetadataFunction {};
}

namespace di::meta {
template<concepts::DeserializationFormat S, typename T>
using DeserializeMetadata = decltype(serialization::deserialize_metadata(in_place_type<T>, in_place_type<S>));
}

namespace di::serialization {
namespace detail {
    struct DeserializeInPlaceFunction {
        template<concepts::Deserializer D, typename T, typename F = meta::DeserializationFormat<D>>
        requires(concepts::TagInvocable<DeserializeInPlaceFunction, F, D&, InPlaceType<T>> ||
                 requires(D&& deserializer) { deserializer.deserialize(in_place_type<T>); })
        constexpr auto operator()(D&& deserializer, InPlaceType<T>) const -> meta::DeserializeResult<D, T> {
            if constexpr (concepts::TagInvocable<DeserializeInPlaceFunction, F, D&, InPlaceType<T>>) {
                return function::tag_invoke(*this, F {}, deserializer, in_place_type<T>);
            } else {
                return deserializer.deserialize(in_place_type<T>);
            }
        }

        template<concepts::Deserializer D, typename T,
                 typename M = meta::DeserializeMetadata<meta::DeserializationFormat<D>, T>>
        requires(!(concepts::TagInvocable<DeserializeInPlaceFunction, D&, InPlaceType<T>> ||
                   requires(D&& deserializer) { deserializer.deserialize(in_place_type<T>); }) &&
                 (concepts::TagInvocable<DeserializeInPlaceFunction, D&, InPlaceType<T>, M> ||
                  requires(D&& deserializer) { deserializer.deserialize(in_place_type<T>, M {}); }))
        constexpr auto operator()(D&& deserializer, InPlaceType<T>) const -> meta::DeserializeResult<D, T> {
            if constexpr (concepts::TagInvocable<DeserializeInPlaceFunction, D&, InPlaceType<T>, M>) {
                return function::tag_invoke(*this, deserializer, in_place_type<T>, M {});
            } else {
                return deserializer.deserialize(in_place_type<T>, M {});
            }
        }

        template<typename Format, concepts::Impl<io::Reader> Reader, typename T, typename... Args>
        requires(concepts::DeserializationFormat<Format, Reader, Args...>)
        constexpr auto operator()(Format format, Reader&& reader, InPlaceType<T>, Args&&... args) const
        requires(requires {
            (*this)(serialization::deserializer(format, util::forward<Reader>(reader)), in_place_type<T>,
                    util::forward<Args>(args)...);
        })
        {
            return (*this)(serialization::deserializer(format, util::forward<Reader>(reader)), in_place_type<T>,
                           util::forward<Args>(args)...);
        }
    };
}

constexpr inline auto deserialize_in_place = detail::DeserializeInPlaceFunction {};

namespace detail {
    struct DeserializableFunction {
        template<concepts::Deserializer D, typename T, typename U = meta::RemoveCVRef<T>>
        constexpr auto operator()(InPlaceType<D>, InPlaceType<T>) const -> bool {
            if constexpr (concepts::TagInvocable<DeserializableFunction, InPlaceType<D>, InPlaceType<U>>) {
                return function::tag_invoke(*this, in_place_type<D>, in_place_type<U>);
            } else {
                return requires { deserialize_in_place(util::declval<D>(), in_place_type<U>); };
            }
        }
    };
}

constexpr inline auto deserializable = detail::DeserializableFunction {};
}

namespace di::concepts {
template<typename T, typename D>
concept Deserializable = Deserializer<D> && serialization::deserializable(in_place_type<D>, in_place_type<T>);
}

namespace di::serialization {
namespace detail {
    template<typename T>
    struct DeserializeFunction {
        template<typename D>
        requires(concepts::Deserializable<T, D>)
        constexpr auto operator()(D&& deserializer) const {
            return deserialize_in_place(deserializer, in_place_type<T>);
        }

        template<typename F, typename R, typename... Args>
        requires(requires {
            deserialize_in_place(F(), util::declval<R>(), in_place_type<T>, util::declval<Args>()...);
        })
        constexpr auto operator()(F format, R&& reader, Args&&... args) const {
            return deserialize_in_place(format, util::forward<R>(reader), in_place_type<T>,
                                        util::forward<Args>(args)...);
        }
    };
}

template<typename T>
constexpr inline auto deserialize = detail::DeserializeFunction<meta::RemoveCVRef<T>> {};
}

namespace di {
using concepts::Deserializable;
using concepts::DeserializationFormat;
using concepts::Deserializer;

using meta::DeserializeMetadata;
using meta::DeserializeResult;

using serialization::deserialize;
using serialization::deserialize_in_place;
using serialization::deserializer;
}
