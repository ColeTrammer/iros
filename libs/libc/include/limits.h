#ifndef _LIMITS_H
#define _LIMITS_H 1

#define CHAR_BIT 8

#define INT_MAX 2147483647
#define INT_MIN -2147483648

#define LONG_MAX 9223372036854775807L
#define LONG_MIN -9223372036854775807L

#define LLONG_MAX ((long long) LONG_MAX)
#define LLONG_MIN ((long long) LONG_MIN)

#define ULLONG_MAX 18446744073709551615ULL

#ifndef __is_kernel

#define PAGESIZE  0x1000
#define PAGE_SIZE PAGESIZE

#define PTHREAD_STACK_MIN (PAGE_SIZE * 4)

#endif /* __is_kernel */

#endif /* _LIMITS_H */