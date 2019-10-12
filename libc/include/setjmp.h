#ifndef _SETJMP_H
#define _SETJMP_H 1

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct __jmp_buf {
    
};

typedef struct __jmp_buf jmp_buf[1];

int setjmp(jmp_buf buf);
void longjmp(jmp_buf buf, int val);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _SETJMP_H */