#pragma once

#include <di/util/move.h>

#define DI_TRY(...)                                           \
    __extension__({                                           \
        auto __result = (__VA_ARGS__);                        \
        if (!__result) {                                      \
            return di::util::move(__result).__try_did_fail(); \
        }                                                     \
        di::util::move(__result).__try_did_succeed();         \
    }).__try_move_out()
