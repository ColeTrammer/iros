#pragma once

#define LIIM_ID(...) __VA_ARGS__
#define LIIM_EMPTY(...)

#define LIIM_PRIMITIVE_CAT(x, y) x##y
#define LIIM_CAT(x, y)           PRIMITIVE_CAT(x, y)

#define LIIM_FIRST(x, ...)     x
#define LIIM_SECOND(x, y, ...) y
#define LIIM_TAIL(x, ...)      __VA_ARGS__

#define LIIM_IS_PROBE(...) LIIM_SECOND(__VA_ARGS__, 0)
#define LIIM_PROBE()       ~, 1

#define LIIM_NOT(x)  LIIM_IS_PROBE(LIIM_PRIMITIVE_CAT(__LIIM_NOT_, x))
#define __LIIM_NOT_0 LIIM_PROBE()

#define LIIM_BOOL(x) LIIM_NOT(LIIM_NOT(x))

#define LIIM_IF(cond)         __LIIM_IF(LIIM_BOOL(cond))
#define __LIIM_IF(cond)       LIIM_PRIMITIVE_CAT(__LIIM_IF_, cond)
#define __LIIM_IF_0(...)      __LIIM_IF_ELSE_0
#define __LIIM_IF_1(...)      __VA_ARGS__ __LIIM_IF_ELSE_1
#define __LIIM_IF_ELSE_0(...) __VA_ARGS__
#define __LIIM_IF_ELSE_1(...)

#define LIIM_NOT_EMPTY(...) \
    __LIIM_IF(LIIM_IS_PAREN(LIIM_FIRST(__VA_ARGS__)))(1)(LIIM_BOOL(LIIM_FIRST(__LIIM_NOT_EMPTY_NO_ARGUMENTS __VA_ARGS__)(0)))
#define __LIIM_NOT_EMPTY_NO_ARGUMENTS(...) LIIM_BOOL(LIIM_FIRST(__VA_ARGS__))

#define LIIM_IS_PAREN(x)     LIIM_IS_PROBE(__LIIM_IS_PAREN x)
#define __LIIM_IS_PAREN(...) LIIM_PROBE()

#define LIIM_EVAL(...)    __LIIM_EVAL1(__LIIM_EVAL1(__LIIM_EVAL1(__VA_ARGS__)))
#define __LIIM_EVAL1(...) __LIIM_EVAL2(__LIIM_EVAL2(__LIIM_EVAL2(__VA_ARGS__)))
#define __LIIM_EVAL2(...) __LIIM_EVAL3(__LIIM_EVAL3(__LIIM_EVAL3(__VA_ARGS__)))
#define __LIIM_EVAL3(...) __LIIM_EVAL4(__LIIM_EVAL4(__LIIM_EVAL4(__VA_ARGS__)))
#define __LIIM_EVAL4(...) __LIIM_EVAL5(__LIIM_EVAL5(__LIIM_EVAL5(__VA_ARGS__)))
#define __LIIM_EVAL5(...) __VA_ARGS__

#define LIIM_DEFER(...)    __VA_ARGS__ __LIIM_DEFER1(LIIM_EMPTY)()
#define __LIIM_DEFER1(...) __VA_ARGS__ __LIIM_DEFER2(LIIM_EMPTY)()
#define __LIIM_DEFER2(...) __VA_ARGS__ __LIIM_DEFER3(LIIM_EMPTY)()
#define __LIIM_DEFER3(...) __VA_ARGS__ __LIIM_DEFER4(LIIM_EMPTY)()
#define __LIIM_DEFER4(...) __VA_ARGS__ __LIIM_DEFER5(LIIM_EMPTY)()
#define __LIIM_DEFER5(...) __VA_ARGS__ __LIIM_DEFER6(LIIM_EMPTY)()
#define __LIIM_DEFER6(...) __VA_ARGS__ LIIM_EMPTY()

#define LIIM_UNWRAP(macro, x) LIIM_IF(LIIM_IS_PAREN(x))(macro x)(macro(x))

#define __LIIM_FOR_EACH(cb, ...) \
    LIIM_IF(LIIM_NOT_EMPTY(__VA_ARGS__))(LIIM_UNWRAP(cb, LIIM_FIRST(__VA_ARGS__)) LIIM_FOR_EACH(cb, LIIM_TAIL(__VA_ARGS__)))()
#define __LIIM_FOR_EACH_INDIRECT() __LIIM_FOR_EACH
#define LIIM_FOR_EACH(cb, ...)     LIIM_DEFER(__LIIM_FOR_EACH_INDIRECT)()(cb, ##__VA_ARGS__)

#define LIIM_COMMA()        ,
#define LIIM_COMMA_IF(cond) LIIM_IF(LIIM_BOOL(cond))(LIIM_COMMA())()

#define LIIM_LIST_INIT(...)           (__VA_ARGS__)
#define LIIM_LIST_EXPAND(...)         __VA_ARGS__
#define LIIM_LIST_JOIN_COMMA(list)    LIIM_ID list
#define LIIM_LIST_NOT_EMPTY(list)     LIIM_NOT_EMPTY list
#define LIIM_LIST_EMPTY(list)         LIIM_NOT(LIIM_LIST_NOT_EMPTY(list))
#define LIIM_LIST_FIRST(list)         LIIM_FIRST list
#define LIIM_LIST_TAIL(list)          (LIIM_TAIL list)
#define LIIM_LIST_PREPEND(list, item) LIIM_IF(LIIM_LIST_NOT_EMPTY(list))((item, LIIM_LIST_EXPAND list))((item))
#define LIIM_LIST_APPEND(list, item)  LIIM_IF(LIIM_LIST_NOT_EMPTY(list))((LIIM_LIST_EXPAND list, item))((item))

#define LIIM_LIST_CONCAT(list1, list2) \
    LIIM_IF(LIIM_LIST_NOT_EMPTY(list1))((LIIM_LIST_EXPAND list1 LIIM_COMMA_IF(LIIM_LIST_NOT_EMPTY(list2)) LIIM_LIST_EXPAND list2))(list2)

#define LIIM_LIST_FOR_EACH(cb, list) LIIM_FOR_EACH(cb, LIIM_LIST_EXPAND list)

#define __LIIM_LIST_REDUCE(cb, acc, list) \
    LIIM_IF(LIIM_LIST_EMPTY(list))        \
    (acc)(LIIM_LIST_REDUCE(cb, cb(acc, LIIM_LIST_FIRST(list)), LIIM_LIST_TAIL(list)))
#define __LIIM_LIST_REDUCE_INDIRECT()   __LIIM_LIST_REDUCE
#define LIIM_LIST_REDUCE(cb, acc, list) LIIM_DEFER(__LIIM_LIST_REDUCE_INDIRECT)()(cb, acc, list)
