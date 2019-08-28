#ifndef _STDIO_H
#define _STDIO_H 1

#include <stdarg.h>
#include <stddef.h>

#define SEEK_SET 0

typedef struct { int empty; } FILE;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern FILE* stderr;
#define stderr stderr

int puts(const char*);
int printf(const char*, ...);

int fclose(FILE*);
int fflush(FILE*);
FILE *fopen(const char*, const char*);
int fprintf(FILE*, const char*, ...);
size_t fread(void*, size_t, size_t, FILE*);

#define SEEK_SET (1)
#define SEEK_CUR (2)
#define SEEK_END (3)
int fseek(FILE*, long, int);
long ftell(FILE*);

size_t fwrite(const void*, size_t, size_t, FILE*);
void setbuf(FILE*, char*);
int vfprintf(FILE*, const char*, va_list);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _STDIO_H */