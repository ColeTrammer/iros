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

FILE* fopen(char const* __CCPP_RESTRICT path, char const* __CCPP_RESTRICT mode);
FILE* freopen(char const* __CCPP_RESTRICT path, char const* __CCPP_RESTRICT mode, FILE* __CCPP_RESTRICT file);
int fclose(FILE* file);
int fflush(FILE* file);
void setbuf(FILE* __CCPP_RESTRICT file, char* __CCPP_RESTRICT buffer);
int setvbuf(FILE* __CCPP_RESTRICT file, char* __CCPP_RESTRICT buffer, int mode, size_t size);

size_t fread(void* __CCPP_RESTRICT buffer, size_t size, size_t count, FILE* __CCPP_RESTRICT file);
size_t fwrite(void const* __CCPP_RESTRICT buffer, size_t size, size_t count, FILE* __CCPP_RESTRICT file);

int fgetc(FILE* file);
int getc(FILE* file);
int fgets(char* __CCPP_RESTRICT str, int count, FILE* __CCPP_RESTRICT file);
int fputc(int ch, FILE* file);
int putc(int ch, FILE* file);
int fputs(char const* __CCPP_RESTRICT str, FILE* __CCPP_RESTRICT file);
int getchar(void);
#ifndef __CCPP_C11
char* gets(char* str);
#endif
int putchar(int ch);
int puts(char const* str);
int ungetc(int ch, FILE* file);

int scanf(char const* __CCPP_RESTRICT format, ...);
int fcanf(FILE* __CCPP_RESTRICT file, char const* __CCPP_RESTRICT format, ...);
int sscanf(const char* __CCPP_RESTRICT buffer, char const* __CCPP_RESTRICT format, ...);

#ifdef __CCPP_C99
int vscanf(char const* __CCPP_RESTRICT format, va_list args);
int vfcanf(FILE* __CCPP_RESTRICT file, char const* __CCPP_RESTRICT format, va_list args);
int vsscanf(const char* __CCPP_RESTRICT buffer, char const* __CCPP_RESTRICT format, va_list args);
#endif

int printf(char const* __CCPP_RESTRICT format, ...);
int fprintf(FILE* __CCPP_RESTRICT file, char const* __CCPP_RESTRICT format, ...);
int sprintf(const char* __CCPP_RESTRICT buffer, char const* __CCPP_RESTRICT format, ...);
#ifdef __CCPP_C99
int snprintf(const char* __CCPP_RESTRICT buffer, size_t size, char const* __CCPP_RESTRICT format, ...);
#endif

int vprintf(char const* __CCPP_RESTRICT format, va_list args);
int vfprintf(FILE* __CCPP_RESTRICT file, char const* __CCPP_RESTRICT format, va_list args);
int vsprintf(const char* __CCPP_RESTRICT buffer, char const* __CCPP_RESTRICT format, va_list args);
#ifdef __CCPP_C99
int vsnprintf(const char* __CCPP_RESTRICT buffer, size_t size, char const* __CCPP_RESTRICT format, va_list args);
#endif

long ftell(FILE* file);
int fgetpos(FILE* __CCPP_RESTRICT file, fpos_t* __CCPP_RESTRICT pos);
int fseek(FILE* file, long offset, int origin);
int fsetpos(FILE* file, fpos_t const* pos);
void rewind(FILE* file);

void clearerr(FILE* file);
int feof(FILE* file);
int ferror(FILE* file);
void perror(char const* string);

int remove(char const* path);
int rename(char const* old_path, char const* new_path);

FILE* tmpfile(void);
char* tmpnam(char* path);

extern FILE* stdin;
extern FILE* stdout;
extern FILE* stderr;

__CCPP_END_DECLARATIONS
