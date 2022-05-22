#pragma once

#define TRY(...)                      \
    ({                                \
        auto _v = (__VA_ARGS__);      \
        if (!_v) {                    \
            return _v.try_did_fail(); \
        }                             \
        _v.try_did_succeed();         \
    }).try_move_out()
