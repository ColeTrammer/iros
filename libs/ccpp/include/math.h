#pragma once

#include <ccpp/bits/config.h>

__CCPP_BEGIN_DECLARATIONS
typedef float float_t;
typedef double double_t;

#define HUGE_VAL __builtin_huge_val()

#if defined(__CCPP_C99) || defined(__CCPP_POSIX_EXTENSIONS)
#define HUGE_VALF __builtin_huge_valf()
#define HUGE_VALL __builtin_huge_vall()

#define INFINITY __builtin_inff()
#define NAN      __builtin_nanf("")

#define FP_NAN       0
#define FP_INFINITE  1
#define FP_ZERO      2
#define FP_SUBNORMAL 3
#define FP_NORMAL    4

#define fpclassify(x) __builtin_fpclassify(FP_NAN, FP_INFINITE, FP_ZERO, FP_SUBNORMAL, FP_NORMAL, x)

#define isfinite(x) __builtin_isfinite(x)
#define isinf(x)    __builtin_isinf_sign(x)
#define isnan(x)    __builtin_isnan(x)
#define isnormal(x) __builtin_isnormal(x)

#define signbit(x)           __builtin_signbit(x)
#define isgreater(x, y)      __builtin_isgreater(x, y)
#define isgreaterequal(x, y) __builtin_isgreaterequal(x, y)
#define isless(x, y)         __builtin_isless(x, y)
#define islessequal(x, y)    __builtin_islessequal(x, y)
#define islessgreater(x, y)  __builtin_islessgreater(x, y)
#define isunordered(x, y)    __builtin_isunordered(x, y)
#endif

double fabs(double __x);
#if defined(__CCPP_C99) || defined(__CCPP_POSIX_EXTENSIONS)
float fabsf(float __x);
long double fabsl(long double __x);
#endif

double fmod(double __x, double __y);
#if defined(__CCPP_C99) || defined(__CCPP_POSIX_EXTENSIONS)
float fmodf(float __x, float __y);
long double fmodl(long double __x, long double __y);
#endif

#if defined(__CCPP_C99) || defined(__CCPP_POSIX_EXTENSIONS)
double remainder(double __x, double __y);
float remainderf(float __x, float __y);
long double remainderl(long double __x, long double __y);

double remquo(double __x, double __y, int* __quo);
float remquof(float __x, float __y, int* __quo);
long double remquol(long double __x, long double __y, int* __quo);

double fma(double __x, double __y, double __z);
float fmaf(float __x, float __y, float __z);
long double fmal(long double __x, long double __y, long double __z);

double fmax(double __x, double __y);
float fmaxf(float __x, float __y);
long double fmaxl(long double __x, long double __y);

double fmin(double __x, double __y);
float fminf(float __x, float __y);
long double fminl(long double __x, long double __y);

double fdim(double __x, double __y);
float fdimf(float __x, float __y);
long double fdiml(long double __x, long double __y);

double nan(char const* __arg);
float nanf(char const* __arg);
long double nanl(char const* __arg);
#endif

double exp(double __x);
#if defined(__CCPP_C99) || defined(__CCPP_POSIX_EXTENSIONS)
float expf(float __x);
long double expl(long double __x);
#endif

#if defined(__CCPP_C99) || defined(__CCPP_POSIX_EXTENSIONS)
double exp2(double __x);
float exp2f(float __x);
long double exp2l(long double __x);

double exmpm1(double __x);
float expm1f(float __x);
long double expm1l(long double __x);
#endif

double log(double __x);
#if defined(__CCPP_C99) || defined(__CCPP_POSIX_EXTENSIONS)
float logf(float __x);
long double logl(long double __x);
#endif

double log10(double __x);
#if defined(__CCPP_C99) || defined(__CCPP_POSIX_EXTENSIONS)
float log10f(float __x);
long double log10l(long double __x);
#endif

#if defined(__CCPP_C99) || defined(__CCPP_POSIX_EXTENSIONS)
double log2(double __x);
float log2f(float __x);
long double log2l(long double __x);

double log1p(double __x);
float log1pf(float __x);
long double log1pl(long double __x);
#endif

double pow(double __x, double __y);
#if defined(__CCPP_C99) || defined(__CCPP_POSIX_EXTENSIONS)
float powf(float __x, float __y);
long double powl(long double __x, long double __y);
#endif

double sqrt(double __x);
#if defined(__CCPP_C99) || defined(__CCPP_POSIX_EXTENSIONS)
float sqrtf(float __x);
long double sqrtl(long double __x);
#endif

#if defined(__CCPP_C99) || defined(__CCPP_POSIX_EXTENSIONS)
double cbrt(double __x);
float cbrtf(float __x);
long double cbrtl(long double __x);

double hypot(double __x, double __y);
float hypotf(float __x, float __y);
long double hypotl(long double __x, long double __y);
#endif

double sin(double __x);
#if defined(__CCPP_C99) || defined(__CCPP_POSIX_EXTENSIONS)
float sinf(float __x);
long double sinl(long double __x);
#endif

double cos(double __x);
#if defined(__CCPP_C99) || defined(__CCPP_POSIX_EXTENSIONS)
float cosf(float __x);
long double cosl(long double __x);
#endif

double tan(double __x);
#if defined(__CCPP_C99) || defined(__CCPP_POSIX_EXTENSIONS)
float tanf(float __x);
long double tanl(long double __x);
#endif

double asin(double __x);
#if defined(__CCPP_C99) || defined(__CCPP_POSIX_EXTENSIONS)
float asinf(float __x);
long double asinl(long double __x);
#endif

double acos(double __x);
#if defined(__CCPP_C99) || defined(__CCPP_POSIX_EXTENSIONS)
float acosf(float __x);
long double acosl(long double __x);
#endif

double atan(double __x);
#if defined(__CCPP_C99) || defined(__CCPP_POSIX_EXTENSIONS)
float atanf(float __x);
long double atanl(long double __x);
#endif

double atan2(double __y, double __x);
#if defined(__CCPP_C99) || defined(__CCPP_POSIX_EXTENSIONS)
float atan2f(float __y, float __x);
long double atan2l(long double __y, long double __x);
#endif

double sinh(double __x);
#if defined(__CCPP_C99) || defined(__CCPP_POSIX_EXTENSIONS)
float sinhf(float __x);
long double sinhl(long double __x);
#endif

double cosh(double __x);
#if defined(__CCPP_C99) || defined(__CCPP_POSIX_EXTENSIONS)
float coshf(float __x);
long double coshl(long double __x);
#endif

double tanh(double __x);
#if defined(__CCPP_C99) || defined(__CCPP_POSIX_EXTENSIONS)
float tanhf(float __x);
long double tanhl(long double __x);
#endif

#if defined(__CCPP_C99) || defined(__CCPP_POSIX_EXTENSIONS)
double asinh(double __x);
float asinhf(float __x);
long double asinhl(long double __x);

double acosh(double __x);
float acoshf(float __x);
long double acoshl(long double __x);

double atanh(double __x);
float atanhf(float __x);
long double atanhl(long double __x);
#endif

#if defined(__CCPP_C99) || defined(__CCPP_POSIX_EXTENSIONS)
double erf(double __x);
float erff(float __x);
long double erfl(long double __x);

double erfc(double __x);
float erfcf(float __x);
long double erfcl(long double __x);

double tgamma(double __x);
float tgammaf(float __x);
long double tgammal(long double __x);

double lgamma(double __x);
float lgammaf(float __x);
long double lgammal(long double __x);
#endif

double ceil(double __x);
#if defined(__CCPP_C99) || defined(__CCPP_POSIX_EXTENSIONS)
float ceilf(float __x);
long double ceill(long double __x);
#endif

double floor(double __x);
#if defined(__CCPP_C99) || defined(__CCPP_POSIX_EXTENSIONS)
float floorf(float __x);
long double floorl(long double __x);
#endif

#if defined(__CCPP_C99) || defined(__CCPP_POSIX_EXTENSIONS)
double trunc(double __x);
float truncf(float __x);
long double truncl(long double __x);

double round(double __x);
float roundf(float __x);
long double roundl(long double __x);

long lround(double __x);
long lroundf(float __x);
long lroundl(long double __x);

long long llround(double __x);
long long llroundf(float __x);
long long llroundl(long double __x);

double nearbyint(double __x);
float nearbyintf(float __x);
long double nearbyintl(long double __x);

double rint(double __x);
float rintf(float __x);
long double rintl(long double __x);

long lrint(double __x);
long lrintf(float __x);
long lrintl(long double __x);

long long llrint(double __x);
long long llrintf(float __x);
long long llrintl(long double __x);
#endif

double frexp(double __x, int* __exp);
#if defined(__CCPP_C99) || defined(__CCPP_POSIX_EXTENSIONS)
float frexpf(float __x, int* __exp);
long double frexpl(long double __x, int* __exp);
#endif

double ldexp(double __x, int __exp);
#if defined(__CCPP_C99) || defined(__CCPP_POSIX_EXTENSIONS)
float ldexpf(float __x, int __exp);
long double ldexpl(long double __x, int __exp);
#endif

double modf(double __x, double* __iptr);
#if defined(__CCPP_C99) || defined(__CCPP_POSIX_EXTENSIONS)
float modff(float __x, float* __iptr);
long double modfl(long double __x, long double* __iptr);
#endif

#if defined(__CCPP_C99) || defined(__CCPP_POSIX_EXTENSIONS)
double scalbn(double __x, int __n);
float scalbnf(float __x, int __n);
long double scalbnl(long double __x, int __n);

double scalbln(double __x, long __n);
float scalblnf(float __x, long __n);
long double scalblnl(long double __x, long __n);

int ilogb(double __x);
int ilogbf(float __x);
int ilogbl(long double __x);

double logb(double __x);
float logbf(float __x);
long double logbl(long double __x);

double nextafter(double __x, double __y);
float nextafterf(float __x, float __y);
long double nextafterl(long double __x, long double __y);

double nexttoward(double __x, long double __y);
float nexttowardf(float __x, long double __y);
long double nexttowardl(long double __x, long double __y);

double copysign(double __x, double __y);
float copysignf(float __x, float __y);
long double copysignl(long double __x, long double __y);
#endif

__CCPP_END_DECLARATIONS
