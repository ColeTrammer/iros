#ifndef _CTYPE_H
#define _CTYPE_H 1

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

int isalnum(int c);
int isalpha(int c);
int isascii(int c);
int isblank(int c);
int iscntrl(int c);
int isdigit(int c);
int isgraph(int c);
int islower(int c);
int isprint(int c);
int ispunct(int c);
int isspace(int c);
int isupper(int c);
int isxdigit(int c);

#define _toupper(c) (c & ~('A' ^ 'a'))
#define _tolower(c) (c | ('A' ^ 'a'))

int toupper(int c);
int tolower(int c);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _CTYPE_H */