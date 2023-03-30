#pragma once

#include <ccpp/bits/config.h>
#include <ccpp/bits/size_t.h>
#include <ccpp/bits/va_list.h>

__CCPP_BEGIN_DECLARATIONS

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

#define _IOFBF 1
#define _IOLBF 2
#define _IONBF 0

#define BUFSIZ 16384

#define EOF (-1)

struct FILE;
typedef struct FILE* FILE;

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
int sprintf(char const* __CCPP_RESTRICT __buffer, char const* __CCPP_RESTRICT __format, ...);
#ifdef __CCPP_C99
int snprintf(char const* __CCPP_RESTRICT __buffer, size_t __size, char const* __CCPP_RESTRICT __format, ...);
#endif

int vprintf(char const* __CCPP_RESTRICT __format, va_list __args);
int vfprintf(FILE* __CCPP_RESTRICT __file, char const* __CCPP_RESTRICT __format, va_list __args);
int vsprintf(char const* __CCPP_RESTRICT __buffer, char const* __CCPP_RESTRICT __format, va_list __args);
#ifdef __CCPP_C99
int vsnprintf(char const* __CCPP_RESTRICT __buffer, size_t __size, char const* __CCPP_RESTRICT __format,
              va_list __args);
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
