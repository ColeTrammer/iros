#pragma once

#include <ipc/stream.h>
#include <stdint.h>

#define __PRIMITIVE_CAT(a, ...) a##__VA_ARGS__

#define __IFF(c)        __PRIMITIVE_CAT(__IFF_, c)
#define __IFF_0(t, ...) __VA_ARGS__
#define __IFF_1(t, ...) t

#define __COMPL(b) __PRIMITIVE_CAT(__COMPL_, b)
#define __COMPL_0  1
#define __COMPL_1  0

#define __CHECK_N(x, n, ...) n
#define __CHECK(...)         __CHECK_N(__VA_ARGS__, 0, )
#define __PROBE(x)           x, 1,

#define __IS_PAREN_PROBE(...) __PROBE(~)
#define __IS_PAREN(x)         __CHECK(__IS_PAREN_PROBE x)

#define __NOT(x) __CHECK(__PRIMITIVE_CAT(__NOT_, x))
#define __NOT_0  __PROBE(~)

#define __BOOL(x) __COMPL(__NOT(x))
#define __IF(c)   __IFF(__BOOL(c))

#define __EMPTY(...)
#define __DEFER(...)    __VA_ARGS__ __EMPTY()
#define __OBSTRUCT(...) __VA_ARGS__ __DEFER(__EMPTY)()

#define __EVAL(...)  __EVAL1(__EVAL1(__EVAL1(__VA_ARGS__)))
#define __EVAL1(...) __EVAL2(__EVAL2(__EVAL2(__VA_ARGS__)))
#define __EVAL2(...) __EVAL3(__EVAL3(__EVAL3(__VA_ARGS__)))
#define __EVAL3(...) __EVAL4(__EVAL4(__EVAL4(__VA_ARGS__)))
#define __EVAL4(...) __EVAL5(__EVAL5(__EVAL5(__VA_ARGS__)))
#define __EVAL5(...) __VA_ARGS__

#define __EVAL_(...)  __EVAL1_(__EVAL1_(__EVAL1_(__VA_ARGS__)))
#define __EVAL1_(...) __EVAL2_(__EVAL2_(__EVAL2_(__VA_ARGS__)))
#define __EVAL2_(...) __EVAL3_(__EVAL3_(__EVAL3_(__VA_ARGS__)))
#define __EVAL3_(...) __EVAL4_(__EVAL4_(__EVAL4_(__VA_ARGS__)))
#define __EVAL4_(...) __EVAL5_(__EVAL5_(__EVAL5_(__VA_ARGS__)))
#define __EVAL5_(...) __VA_ARGS__

#define __EVAL__(...)  __EVAL1__(__EVAL1__(__EVAL1__(__VA_ARGS__)))
#define __EVAL1__(...) __EVAL2__(__EVAL2__(__EVAL2__(__VA_ARGS__)))
#define __EVAL2__(...) __EVAL3__(__EVAL3__(__EVAL3__(__VA_ARGS__)))
#define __EVAL3__(...) __EVAL4__(__EVAL4__(__EVAL4__(__VA_ARGS__)))
#define __EVAL4__(...) __EVAL5__(__EVAL5__(__EVAL5__(__VA_ARGS__)))
#define __EVAL5__(...) __VA_ARGS__

#define __FIRST(x, ...) x
#define __NEXT(x, ...)  __VA_ARGS__

#define __RECURSIVE_ITER_INDIRECT() __RECURSIVE_ITER
#define __RECURSIVE_ITER(op, ...)          \
    __IF(__IS_PAREN(__FIRST(__VA_ARGS__))) \
    (op __FIRST(__VA_ARGS__) __OBSTRUCT(__RECURSIVE_ITER_INDIRECT)()(op, __NEXT(__VA_ARGS__)), __VA_ARGS__)

#define __RECURSIVE_ITER__INDIRECT() __RECURSIVE_ITER_
#define __RECURSIVE_ITER_(op, ...)         \
    __IF(__IS_PAREN(__FIRST(__VA_ARGS__))) \
    (__EVAL_(op __FIRST(__VA_ARGS__)) __OBSTRUCT(__RECURSIVE_ITER__INDIRECT)()(op, __NEXT(__VA_ARGS__)), __VA_ARGS__)

#define __MESSAGE_FIELD_DECL(type, name)               type name;
#define __MESSAGE_FIELD_SERIALIZATION_SIZE(type, name) ret += IPC::Serializer<type>::serialization_size(name);
#define __MESSAGE_FIELD_SERIALIZER(type, name)         stream << name;
#define __MESSAGE_FIELD_DESERIALIZER(type, name)       stream >> name;

#define __MESSAGE_DECL(...)                    __EVAL(__RECURSIVE_ITER(__MESSAGE_FIELD_DECL __VA_OPT__(, ) __VA_ARGS__))
#define __MESSAGE_SERIALIZATION_SIZE_BODY(...) __EVAL(__RECURSIVE_ITER(__MESSAGE_FIELD_SERIALIZATION_SIZE __VA_OPT__(, ) __VA_ARGS__))
#define __MESSAGE_SERIALIZER_BODY(...)         __EVAL(__RECURSIVE_ITER(__MESSAGE_FIELD_SERIALIZER __VA_OPT__(, ) __VA_ARGS__))
#define __MESSAGE_SERIALIZER(n, ...)                   \
    uint32_t serialization_size() const {              \
        uint32_t ret = 2 * sizeof(uint32_t);           \
        __MESSAGE_SERIALIZATION_SIZE_BODY(__VA_ARGS__) \
        return ret;                                    \
    }                                                  \
    bool serialize(IPC::Stream& stream) {              \
        stream << serialization_size();                \
        stream << static_cast<uint32_t>(Type::n);      \
        __MESSAGE_SERIALIZER_BODY(__VA_ARGS__)         \
        return !stream.error();                        \
    }

#define __MESSAGE_DESERIALIZER_BODY(...) __EVAL(__RECURSIVE_ITER(__MESSAGE_FIELD_DESERIALIZER __VA_OPT__(, ) __VA_ARGS__))
#define __MESSAGE_DESERIALIZER(n, ...)                \
    bool deserialize(IPC::Stream& stream) {           \
        uint32_t size;                                \
        uint32_t type;                                \
        stream >> size;                               \
        stream >> type;                               \
        if (type != static_cast<uint32_t>(Type::n)) { \
            return false;                             \
        }                                             \
        __MESSAGE_DESERIALIZER_BODY(__VA_ARGS__)      \
        if (size != serialization_size()) {           \
            return false;                             \
        }                                             \
        return !stream.error();                       \
    }

#define __MESSAGE_BEGIN(n) struct n {
#define __MESSAGE_BODY(n, ...) \
    __MESSAGE_DECL(__VA_ARGS__) __MESSAGE_SERIALIZER(n __VA_OPT__(, ) __VA_ARGS__) __MESSAGE_DESERIALIZER(n __VA_OPT__(, ) __VA_ARGS__)
#define __MESSAGE_END(n) }

#define __MESSAGE_TYPE(n, ...) n,
#define __MESSAGE(n, ...)      __MESSAGE_BEGIN(n) __MESSAGE_BODY(n, __VA_ARGS__) __MESSAGE_END(n);

#define __IPC_MESSAGE_TYPES(...)                                                             \
    enum class Type : uint32_t {                                                             \
        __EVAL__(__RECURSIVE_ITER_(__MESSAGE_TYPE __VA_OPT__(, ) __VA_ARGS__)) MessageCount, \
    };
#define __IPC_MESSAGES(...) __EVAL__(__RECURSIVE_ITER_(__MESSAGE __VA_OPT__(, ) __VA_ARGS__))

#define IPC_MESSAGES(n, ...)         \
    namespace n {                    \
    __IPC_MESSAGE_TYPES(__VA_ARGS__) \
    __IPC_MESSAGES(__VA_ARGS__)      \
    }
