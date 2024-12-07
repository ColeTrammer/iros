#pragma once

#include "di/container/concepts/container_of.h"
#include "di/container/concepts/forward_container.h"
#include "di/container/string/encoding.h"
#include "di/meta/core.h"
#include "di/meta/language.h"
#include "di/types/size_t.h"

namespace di::concepts {
template<typename T>
concept ParserContext = concepts::ContainerOf<T, c32> && concepts::HasEncoding<T> && concepts::ForwardContainer<T> &&
                        requires(T& context, meta::ContainerIterator<T> it) {
                            typename T::Error;

                            { context.encoding() } -> SameAs<meta::Encoding<T>>;
                            { context.advance(it) } -> LanguageVoid;
                            { context.make_error() } -> SameAs<typename T::Error>;
                        };
}
