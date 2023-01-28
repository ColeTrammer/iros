#ifndef _STDIO_H
#define _STDIO_H 1

#include <bits/file.h>
#include <bits/lock.h>
#include <bits/off_t.h>
#include <bits/seek_constants.h>
#include <bits/ssize_t.h>
#include <bits/va_list.h>
#include <stddef.h>

#define BUFSIZ 0x1000

#define _IOFBF 0
#define _IOLBF 1
#define _IONBF 2

#define FOPEN_MAX 16

#define L_tmpnam 16
#define P_tmpdir "/tmp"
#define TMP_MAX  25

#define EOF (-1)

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef long fpos_t;

#ifndef __is_libk

extern FILE *__file_list_head;
extern FILE *__file_list_tail;
extern struct __lock __file_list_lock;
extern int __p_child_pid;

struct __stdio_flags __stdio_parse_mode_string(const char *s);
FILE *__stdio_allocate_stream(int fd, int flags);

#ifdef NDEBUG
#define __stdio_log(ignore, s, ...) ((void) 0)
#else
extern int __should_log;
#define __stdio_log(stream, s, ...)             \
    do {                                        \
        if (__should_log && stream != stderr) { \
            fprintf(stderr, s, ##__VA_ARGS__);  \
            fputc('\n', stderr);                \
        }                                       \
    } while (0)
#endif /* NDEBUG */

extern FILE *stdout;
#define stdout stdout

extern FILE *stdin;
#define stdin stdin

extern FILE *stderr;
#define stderr stderr

#else
typedef struct {
} FILE;
#endif /* __is_libk */

int fclose(FILE *stream);
int fflush(FILE *stream);
FILE *fopen(const char *__restrict path, const char *__restrict mode);
FILE *fdopen(int fd, const char *mode);
FILE *freopen(const char *path, const char *mode, FILE *stream);
void setbuf(FILE *__restrict stream, char *__restrict buf);
int setvbuf(FILE *__restrict stream, char *__restrict buf, int mode, size_t size);

int dprintf(int fd, const char *__restrict format, ...) __attribute__((format(printf, 2, 3)));
int fprintf(FILE *stream, const char *__restrict format, ...) __attribute__((format(printf, 2, 3)));
int fscanf(FILE *stream, const char *__restrict format, ...) __attribute__((format(scanf, 2, 3)));
int printf(const char *__restrict format, ...) __attribute__((format(printf, 1, 2)));
int scanf(const char *__restrict format, ...) __attribute__((format(scanf, 1, 2)));
int snprintf(char *__restrict str, size_t size, const char *__restrict format, ...) __attribute__((format(printf, 3, 4)));
int sprintf(char *__restrict str, const char *__restrict format, ...) __attribute__((format(printf, 2, 3)));
int sscanf(const char *__restrict src, const char *__restrict format, ...) __attribute__((format(scanf, 2, 3)));

int vdprintf(int fd, const char *__restrict format, va_list args) __attribute__((format(printf, 2, 0)));
int vfprintf(FILE *stream, const char *__restrict format, va_list args) __attribute__((format(printf, 2, 0)));
int vfscanf(FILE *stream, const char *__restrict format, va_list args) __attribute__((format(scanf, 2, 0)));
int vprintf(const char *__restrict format, va_list args) __attribute__((format(printf, 1, 0)));
int vscanf(const char *__restrict format, va_list args) __attribute__((format(scanf, 1, 0)));
int vsnprintf(char *__restrict str, size_t size, const char *__restrict format, va_list args) __attribute__((format(printf, 3, 0)));
int vsprintf(char *__restrict str, const char *__restrict format, va_list args) __attribute__((format(printf, 2, 0)));
int vsscanf(const char *__restrict src, const char *__restrict format, va_list args) __attribute__((format(scanf, 2, 0)));

int fgetc(FILE *stream);
char *fgets(char *__restrict s, int size, FILE *__restrict stream);
int fputc(int c, FILE *stream);
int fputs(const char *s, FILE *stream);
int getc(FILE *stream);
int getchar(void);
char *gets(char *s) __attribute__((deprecated));
int putc(int c, FILE *stream);
int putchar(int c);
int puts(const char *s);
int ungetc(int c, FILE *stream);

size_t fread(void *buf, size_t size, size_t nmemb, FILE *stream);
size_t fwrite(const void *buf, size_t size, size_t nmemb, FILE *stream);

int fgetpos(FILE *__restrict stream, fpos_t *__restrict pos);
int fseek(FILE *stream, long offset, int whence);
int fseeko(FILE *stream, off_t offset, int whence);
int fsetpos(FILE *__restrict stream, const fpos_t *__restrict pos);
long ftell(FILE *stream);
off_t ftello(FILE *stream);
void rewind(FILE *stream);

void clearerr(FILE *stream);
int feof(FILE *stream);
int ferror(FILE *stream);
int fileno(FILE *stream);

void clearerr_unlocked(FILE *stream);
int fflush_unlocked(FILE *stream);
int feof_unlocked(FILE *stream);
int ferror_unlocked(FILE *stream);
int fgetc_unlocked(FILE *stream);
char *fgets_unlocked(char *__restrict, int size, FILE *__restrict stream);
int fileno_unlocked(FILE *stream);
int fputc_unlocked(int c, FILE *stream);
int fputs_unlocked(const char *__restrict s, FILE *__restrict stream);
size_t fread_unlocked(void *__restrict buf, size_t size, size_t nmemb, FILE *__restrict stream);
size_t fwrite_unlocked(const void *__restrict buf, size_t size, size_t nmemb, FILE *__restrict stream);
int getc_unlocked(FILE *stream);
int getchar_unlocked(void);
int putc_unlocked(int c, FILE *stream);
int putchar_unlocked(int c);
int fseek_unlocked(FILE *stream, long offset, int whence);
int fseeko_unlocked(FILE *stream, off_t offset, int whence);
long ftell_unlocked(FILE *stream);
off_t ftello_unlocked(FILE *stream);

void flockfile(FILE *stream);
int ftrylockfile(FILE *stream);
void funlockfile(FILE *stream);

int remove(const char *path);
int rename(const char *old_path, const char *new_path);

ssize_t getdelim(char **__restrict line_ptr, size_t *__restrict n, int delim, FILE *__restrict stream);
ssize_t getline(char **__restrict line_ptr, size_t *__restrict n, FILE *__restrict stream);

char *tmpnam(char *s);
FILE *tmpfile(void);

FILE *popen(const char *command, const char *mode);
int pclose(FILE *stream);

void perror(const char *s);

#ifdef __libc_internal

void init_files(int isatty_mask);

#endif /* __libc_internal */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _STDIO_H */
