#ifndef _SETJMP_H
#define _SETJMP_H 1

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifdef __x86_64__
struct __jmp_buf {
    unsigned long registers[8]; // 7 Saved for SYS V ABI and 1 for RIP
    int is_mask_saved;
    __UINT64_TYPE__ mask;
};
#elif defined(__i386__)
struct __jmp_buf {
    unsigned long registers[6]; // 5 Saved for SYS V ABI and 1 for RIP
    int is_mask_saved;
    __UINT64_TYPE__ mask;
};
#else
#error "Unsupported architecture"
#endif

typedef struct __jmp_buf jmp_buf[1];
typedef struct __jmp_buf sigjmp_buf[1];

int setjmp(jmp_buf buf);
__attribute__((noreturn)) void longjmp(jmp_buf buf, int val);

int sigsetjmp(sigjmp_buf env, int val);
__attribute__((noreturn)) void siglongjmp(sigjmp_buf env, int val);

int _setjmp(jmp_buf buf);
__attribute__((noreturn)) void _longjmp(jmp_buf buf, int val);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _SETJMP_H */
