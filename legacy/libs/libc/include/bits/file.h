#ifndef _BITS_FILE_H
#define _BITS_FILE_H 1

#include <bits/lock.h>
#include <bits/off_t.h>
#include <bits/size_t.h>

#ifndef __is_libk

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define __STDIO_OWNED                 4
#define __STDIO_READABLE              8
#define __STDIO_WRITABLE              16
#define __STDIO_ERROR                 32
#define __STDIO_EOF                   64
#define __STDIO_DYNAMICALLY_ALLOCATED 128
#define __STDIO_APPEND                256
#define __STDIO_HAS_UNGETC_CHARACTER  512
#define __STDIO_LAST_OP_READ          1024
#define __STDIO_LAST_OP_WRITE         2048

struct __stdio_flags {
    int __stream_flags;
    int __open_flags;
};

struct __file {
    struct __file *__next;
    struct __file *__prev;
    char *__buffer;
    size_t __buffer_max;
    size_t __buffer_length;
    off_t __position;
    int __fd;
    int __flags;
    int __ungetc_character;
    struct __recursive_lock __lock;
};

typedef struct __file FILE;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __is_libk */

#endif /* _BITS_FILE_H */
