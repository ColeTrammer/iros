#pragma once

#include <di/sync/concepts/stoppable_token.h>

namespace di::concepts {
template<typename T>
concept UnstoppableToken = StoppableToken<T> && requires {
                                                    { T::stop_possible() } -> BooleanTestable;
                                                } && (!T::stop_possible());
}