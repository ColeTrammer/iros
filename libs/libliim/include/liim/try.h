#pragma once

#define TRY(option)                   \
    ({                                \
        auto _v = (option);           \
        if (!_v) {                    \
            return _v.try_did_fail(); \
        }                             \
        _v.try_did_succeed();         \
    }).try_move_out()
