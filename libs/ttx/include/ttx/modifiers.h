#pragma once

#include "di/reflect/enumerator.h"
#include "di/reflect/reflect.h"
#include "di/util/bitwise_enum.h"

// Key reference: https://sw.kovidgoyal.net/kitty/keyboard-protocol/
namespace ttx {
enum class Modifiers {
    None = 0,
    Shift = 1 << 0,
    Alt = 1 << 1,
    Control = 1 << 2,
    Super = 1 << 3,
    Hyper = 1 << 4,
    Meta = 1 << 5,
    CapsLock = 1 << 6,
    NumLock = 1 << 7,
    LockModifiers = CapsLock | NumLock,
};

DI_DEFINE_ENUM_BITWISE_OPERATIONS(Modifiers)

constexpr auto tag_invoke(di::Tag<di::reflect>, di::InPlaceType<Modifiers>) {
    using enum Modifiers;
    return di::make_enumerators<"Modifiers">(
        di::enumerator<"None", None>, di::enumerator<"Shift", Shift>, di::enumerator<"Alt", Alt>,
        di::enumerator<"Control", Control>, di::enumerator<"Super", Super>, di::enumerator<"Hyper", Hyper>,
        di::enumerator<"Meta", Meta>, di::enumerator<"CapsLock", CapsLock>, di::enumerator<"NumsLock", NumLock>);
}
}
