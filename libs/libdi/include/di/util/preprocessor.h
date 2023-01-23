#pragma once

#define DI_ID(...) __VA_ARGS__
#define DI_EMPTY(...)

#define DI_PRIMITIVE_CAT(x, y) x##y
#define DI_CAT(x, y)           DI_PRIMITIVE_CAT(x, y)

#define DI_FIRST(x, ...)     x
#define DI_SECOND(x, y, ...) y
#define DI_TAIL(x, ...)      __VA_ARGS__

#define DI_IS_PROBE(...) DI_SECOND(__VA_ARGS__, 0)
#define DI_PROBE()       ~, 1

#define DI_NOT(x)  DI_IS_PROBE(DI_PRIMITIVE_CAT(__DI_NOT_, x))
#define __DI_NOT_0 DI_PROBE()

#define DI_BOOL(x) DI_NOT(DI_NOT(x))

#define DI_IF(cond)         __DI_IF(DI_BOOL(cond))
#define __DI_IF(cond)       DI_PRIMITIVE_CAT(__DI_IF_, cond)
#define __DI_IF_0(...)      __DI_IF_ELSE_0
#define __DI_IF_1(...)      __VA_ARGS__ __DI_IF_ELSE_1
#define __DI_IF_ELSE_0(...) __VA_ARGS__
#define __DI_IF_ELSE_1(...)