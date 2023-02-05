#pragma once

#include <di/any/concepts/any_storage.h>
#include <di/types/prelude.h>
#include <di/util/create.h>
#include <di/util/defer_construct.h>

namespace di::concepts {
template<typename T, typename Storage>
concept AnyStorableInfallibly =
    AnyStorage<Storage> && requires {
                               requires !T::creation_is_fallible(in_place_type<T>);
                               {
                                   util::create<Storage>(in_place_type<T>, util::DeferConstruct([] -> T {}))
                                   } -> SameAs<Storage>;
                           };
}