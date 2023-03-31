#pragma once

#include <ccpp/bits/config.h>
#include <ccpp/bits/size_t.h>
#include <ccpp/bits/va_list.h>

#include __CCPP_PLATFORM_PATH(seek_constants.h)

__CCPP_BEGIN_DECLARATIONS

#define _IOFBF 1
#define _IOLBF 2
#define _IONBF 0

#define BUFSIZ 16384

#define FILENAME_MAX 4096
#define FOPEN_MAX    1024
#define TMP_MAX      100000
#define L_tmpnam     32

#define EOF (-1)

struct __file_implementation;
typedef struct __file_implementation FILE;

typedef __UINT64_TYPE__ fpos_t;

FILE* fopen(char const* __CCPP_RESTRICT __path, char const* __CCPP_RESTRICT __mode);
FILE* freopen(char const* __CCPP_RESTRICT __path, char const* __CCPP_RESTRICT __mode, FILE* __CCPP_RESTRICT __file);
int fclose(FILE* __file);
int fflush(FILE* __file);
void setbuf(FILE* __CCPP_RESTRICT __file, char* __CCPP_RESTRICT __buffer);
int setvbuf(FILE* __CCPP_RESTRICT __file, char* __CCPP_RESTRICT __buffer, int __mode, size_t __size);

size_t fread(void* __CCPP_RESTRICT __buffer, size_t __size, size_t __count, FILE* __CCPP_RESTRICT __file);
size_t fwrite(void const* __CCPP_RESTRICT __buffer, size_t __size, size_t __count, FILE* __CCPP_RESTRICT __file);

int fgetc(FILE* __file);
int getc(FILE* __file);
int fgets(char* __CCPP_RESTRICT __str, int __count, FILE* __CCPP_RESTRICT __file);
int fputc(int __ch, FILE* __file);
int putc(int __ch, FILE* __file);
int fputs(char const* __CCPP_RESTRICT __str, FILE* __CCPP_RESTRICT __file);
int getchar(void);
#ifndef __CCPP_C11
char* gets(char* __str);
#endif
int putchar(int __ch);
int puts(char const* __str);
int ungetc(int __ch, FILE* __file);

void clearerr(FILE* __file);
int feof(FILE* __file);
int ferror(FILE* __file);

#ifdef __CCPP_POSIX_EXTENSIONS
int getc_unlocked(FILE* __file);
int getchar_unlocked(void);
int putc_unlocked(int __ch, FILE* __file);
int putchar_unlocked(int __ch);
#endif

#ifdef __CCPP_COMPAT
void clearerr_unlocked(FILE* __file);
int feof_unlocked(FILE* __file);
int ferror_unlocked(FILE* __file);
int fflush_unlocked(FILE* __file);

int fputc_unlocked(int __ch, FILE* __file);
int fgetc_unlocked(FILE* __file);

size_t fread_unlocked(void* __CCPP_RESTRICT __buffer, size_t __size, size_t __count, FILE* __CCPP_RESTRICT __file);
size_t fwrit_unlocked(void const* __CCPP_RESTRICT __buffer, size_t __size, size_t __count,
                      FILE* __CCPP_RESTRICT __file);

int fgets_unlocked(char* __CCPP_RESTRICT __str, int __count, FILE* __CCPP_RESTRICT __file);
int fputs_unlocked(char const* __CCPP_RESTRICT __str, FILE* __CCPP_RESTRICT __file);
#endif

int scanf(char const* __CCPP_RESTRICT __format, ...);
int fcanf(FILE* __CCPP_RESTRICT __file, char const* __CCPP_RESTRICT __format, ...);
int sscanf(char const* __CCPP_RESTRICT __buffer, char const* __CCPP_RESTRICT __format, ...);

#ifdef __CCPP_C99
int vscanf(char const* __CCPP_RESTRICT __format, va_list __args);
int vfcanf(FILE* __CCPP_RESTRICT __file, char const* __CCPP_RESTRICT __format, va_list __args);
int vsscanf(char const* __CCPP_RESTRICT __buffer, char const* __CCPP_RESTRICT __format, va_list __args);
#endif

int printf(char const* __CCPP_RESTRICT __format, ...);
int fprintf(FILE* __CCPP_RESTRICT __file, char const* __CCPP_RESTRICT __format, ...);
int sprintf(char* __CCPP_RESTRICT __buffer, char const* __CCPP_RESTRICT __format, ...);
#ifdef __CCPP_C99
int snprintf(char* __CCPP_RESTRICT __buffer, size_t __size, char const* __CCPP_RESTRICT __format, ...);
#endif

int vprintf(char const* __CCPP_RESTRICT __format, va_list __args);
int vfprintf(FILE* __CCPP_RESTRICT __file, char const* __CCPP_RESTRICT __format, va_list __args);
int vsprintf(char* __CCPP_RESTRICT __buffer, char const* __CCPP_RESTRICT __format, va_list __args);
#ifdef __CCPP_C99
int vsnprintf(char* __CCPP_RESTRICT __buffer, size_t __size, char const* __CCPP_RESTRICT __format, va_list __args);
#endif

long ftell(FILE* __file);
int fgetpos(FILE* __CCPP_RESTRICT __file, fpos_t* __CCPP_RESTRICT __pos);
int fseek(FILE* __file, long __offset, int __origin);
int fsetpos(FILE* __file, fpos_t const* __pos);
void rewind(FILE* __file);

void clearerr(FILE* __file);
int feof(FILE* __file);
int ferror(FILE* __file);
void perror(char const* __str);

int remove(char const* __path);
int rename(char const* __old_path, char const* __new_path);

FILE* tmpfile(void);
char* tmpnam(char* __path);

extern FILE* stdin;
extern FILE* stdout;
extern FILE* stderr;

__CCPP_END_DECLARATIONS
