#pragma once

#define TRY(option)                   \
    ({                                \
        auto _v = (option);           \
        if (!_v) {                    \
            return _v.try_did_fail(); \
        }                             \
        move(_v.value());             \
    })
