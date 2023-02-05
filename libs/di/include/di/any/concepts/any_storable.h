#pragma once

#include <di/any/concepts/any_storage.h>
#include <di/types/prelude.h>
#include <di/util/create.h>
#include <di/util/defer_construct.h>
#include <di/util/unreachable.h>

namespace di::concepts {
template<typename T, typename Storage>
concept AnyStorable = AnyStorage<Storage> && requires {
                                                 util::create<Storage>(in_place_type<T>, util::DeferConstruct([] -> T {
                                                                           util::unreachable();
                                                                       }));
                                             };
}