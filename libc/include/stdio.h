#ifndef _STDIO_H
#define _STDIO_H 1

#include <stdarg.h>
#include <stddef.h>
#include <sys/types.h>

#define SEEK_SET (0)
#define SEEK_CUR (1)
#define SEEK_END (2)

#define FOPEN_MAX 8

#define EOF (-1)

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef unsigned long fpos_t;

typedef struct { 
    char *buffer;
    fpos_t pos;
    off_t length;
    int flags;
    int file;
    mode_t mode;
} FILE;

#ifndef __libc_files_c

extern FILE *stdio;
#define stdio stdio

extern FILE *stdin;
#define stdin stdin

extern FILE* stderr;
#define stderr stderr

#endif /* __libc_files_c */

int puts(const char*);
int printf(const char *__restrict format, ...) __attribute__((format (printf, 1, 2)));
int vprintf(const char *__restrict format, va_list args);

int fclose(FILE*);
int fflush(FILE*);
FILE *fopen(const char *__restrict, const char *__restrict);
int fprintf(FILE*, const char*, ...) __attribute__((format (printf, 2, 3)));
size_t fread(void*, size_t, size_t, FILE*);

int fseek(FILE*, long, int);
long ftell(FILE*);

size_t fwrite(const void*, size_t, size_t, FILE*);
void setbuf(FILE*, char*);
int vfprintf(FILE*, const char*, va_list);

#ifdef __libc_internal

void init_files();

#endif /* __libc_internal */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _STDIO_H */