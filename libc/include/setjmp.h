#ifndef _SETJMP_H
#define _SETJMP_H 1

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct __jmp_buf {
    uint64_t registers[8]; // 7 Saved for SYS V ABI and 1 for RIP
} __attribute__((packed));

typedef struct __jmp_buf jmp_buf[1];

int setjmp(jmp_buf buf);
__attribute__((noreturn))
void longjmp(jmp_buf buf, int val);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _SETJMP_H */