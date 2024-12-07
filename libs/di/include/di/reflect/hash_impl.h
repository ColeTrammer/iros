#pragma once

#include "di/container/hash/hash_write.h"
#include "di/reflect/reflect.h"

namespace di::container::detail {
constexpr void tag_invoke(types::Tag<container::hash_write>, concepts::Hasher auto& hasher,
                          concepts::ReflectableToFields auto const& object) {
    vocab::tuple_for_each(
        [&](auto field) {
            container::hash_write(hasher, field.get(object));
        },
        reflection::reflect(object));
}
}
