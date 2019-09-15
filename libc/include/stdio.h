#ifndef _STDIO_H
#define _STDIO_H 1

#include <stdarg.h>
#include <stddef.h>
#include <sys/types.h>

#define BUFSIZ 0x1000

#define _IOFBF 0
#define _IOLBF 1
#define _IONBF 2

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
    
    int fd;
    int flags;
    int eof;
    int error;

    int buf_type;

    /* Needs lock for threads eventually */
} FILE;

#ifndef __is_libk

extern FILE *stdout;
#define stdout stdout

extern FILE *stdin;
#define stdin stdin

extern FILE* stderr;
#define stderr stderr

#endif /* __is_libk */

int fclose(FILE *stream);
int fflush(FILE *stream);
FILE *fopen(const char *__restrict path, const char *__restrict mode);
FILE* freopen(const char *path, const char *mode, FILE *stream);
void setbuf(FILE *stream, char *buf);
int setvbuf(FILE *stream, char *buf, int mode, size_t size);

int fprintf(FILE *stream, const char *__restrict format, ...) __attribute__((format (printf, 2, 3)));
int fscanf(FILE* stream, const char *__restrict format, ...) __attribute__((format (scanf, 2, 3)));
int printf(const char *__restrict format, ...) __attribute__((format (printf, 1, 2)));
int scanf(const char *__restrict format, ...) __attribute__((format (scanf, 1, 2)));
int snprintf(char *__restrict str, size_t size, const char *__restrict format, ...) __attribute__((format (printf, 3, 4)));
int sprintf(char *__restrict str, const char *__restrict format, ...) __attribute__((format (printf, 2, 3)));
int sscanf(const char *__restrict src, const char *__restrict format, ...) __attribute__((format (scanf, 2, 3)));

int vfprintf(FILE *stream, const char *__restrict format, va_list args) __attribute__((format (printf, 2, 0)));
int vfscanf(FILE* stream, const char *__restrict format, va_list args) __attribute__((format (scanf, 2, 0)));
int vprintf(const char *__restrict format, va_list args) __attribute__((format (printf, 1, 0)));
int vscanf(const char *__restrict format, va_list args) __attribute__((format (scanf, 1, 0)));
int vsnprintf(char *__restrict str, size_t size, const char *__restrict format, va_list args) __attribute__((format (printf, 3, 0)));
int vsprintf(char *__restrict str, const char *__restrict format, va_list args) __attribute__((format (printf, 2, 0)));
int vsscanf(const char *__restrict src, const char *__restrict format, va_list args) __attribute__((format (scanf, 2, 0)));

int fgetc(FILE *stream);
char *fgets(char *s, int size, FILE *stream);
int fputc(int c, FILE *stream);
int fputs(const char *s, FILE *stream);
#define getc(stream) fgetc(stream)
int getchar();
char *gets(char *s) __attribute__((deprecated));
#define putc(c, stream) fputc(c, stream)
int putchar(int c);
int puts(const char *s);
int ungetc(int c, FILE *stream);

size_t fread(void *buf, size_t size, size_t nmemb, FILE *stream);
size_t fwrite(const void *buf, size_t size, size_t nmemb, FILE *stream);

int fgetpos(FILE *stream, fpos_t *pos);
int fseek(FILE *stream, long offset, int whence);
int fsetpos(FILE *stream, const fpos_t *pos);
long ftell(FILE *stream);
void rewind(FILE *stream);

void clearerr(FILE *stream);
int feof(FILE *stream);
int ferror(FILE *stream);
int fileno(FILE *stream);

void perror(const char *s);

#ifdef __libc_internal

void init_files();

#endif /* __libc_internal */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _STDIO_H */