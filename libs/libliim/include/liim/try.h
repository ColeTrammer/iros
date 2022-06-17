#pragma once

#define TRY(...)                            \
    ({                                      \
        auto _v = (__VA_ARGS__);            \
        if (!_v) {                          \
            return move(_v).try_did_fail(); \
        }                                   \
        move(_v).try_did_succeed();         \
    }).try_move_out()
