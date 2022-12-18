#pragma once

#include <di/concepts/language_void.h>
#include <di/concepts/same_as.h>
#include <di/container/concepts/container_of.h>
#include <di/container/concepts/forward_container.h>
#include <di/types/size_t.h>

namespace di::concepts {
template<typename T>
concept ParserContext =
    concepts::ContainerOf<T, c32> && concepts::ForwardContainer<T> && requires(T& context, meta::ContainerIterator<T> it) {
                                                                          typename T::Error;

                                                                          { context.advance(it) } -> LanguageVoid;
                                                                          { context.make_error() } -> SameAs<typename T::Error>;
                                                                      };
}