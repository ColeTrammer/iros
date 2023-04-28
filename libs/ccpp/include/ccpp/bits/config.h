#pragma once

#ifdef __cplusplus
#define __CCPP_BEGIN_DECLARATIONS extern "C" {
#define __CCPP_END_DECLARATIONS   }
#else
#define __CCPP_BEGIN_DECLARATIONS
#define __CCPP_END_DECLARATIONS
#endif

#if !defined(_POSIX_SOURCE) && !defined(_POSIX_C_SOURCE) && !defined(_XOPEN_SOURCE) && !defined(__STRICT_ANSI__)
#define _GNU_SOURCE 1
#endif

#if defined(_GNU_SOURCE) || defined(_BSD_SOURCE)
#define __CCPP_COMPAT 1
#endif

#if defined(_XOPEN_SOURCE) || defined(_POSIX_SOURCE) || defined(_POSIX_C_SOURCE) || defined(__CCPP_COMPAT)
#define __CCPP_POSIX_EXTENSIONS 1
#endif

#define __CCPP_C23_DEPRECATED

#ifdef __cplusplus
#if (__cplusplus >= 201703L)
#define __CCPP_C11
#endif
#if (__cplusplus >= 201402L)
#define __CCPP_C99
#endif
#define __CCPP_C89
#elif defined(__STDC__)
#ifdef __STDC_VERSION__
#if (__STDC_VERSION__ >= 201710L)
#define __CCPP_C17
#endif
#if (__STDC_VERSION__ >= 201112L)
#define __CCPP_C11
#endif
#if (__STDC_VERSION__ >= 199901L)
#define __CCPP_C99
#endif
#if (__STDC_VERSION__ >= 199409L)
#define __CCPP_C95
#endif
#define __CCPP_C89
#endif
#endif

#if !defined(__cplusplus) && defined(__CCPP_C11)
#define __CCPP_NORETURN _Noreturn
#else
#define __CCPP_NORETURN __attribute__((__noreturn__))
#endif

#if !defined(__cplusplus) && defined(__CCPP_C99)
#define __CCPP_RESTRICT restrict
#elif defined(__GNUC__) || defined(__clang__)
#define __CCPP_RESTRICT __restrict
#else
#define __CCPP_RESTRICT
#endif

#ifdef __x86_64__
#define __CCPP_ARCH x86_64
#define __CCPP_ARCH_X86_64
#else
#error "Unsupported ccpp architecture"
#endif

#ifdef __iros__
#define __CCPP_PLATFORM      iros
#define __CCPP_PLATFORM      iros
#define __CCPP_PLATFORM_IROS 1
#elif defined(__linux__)
#undef __GLIBC__
#undef linux
#define __CCPP_PLATFORM       linux
#define __CCPP_PLATFORM_LINUX 1
#else
#error "Unsupported ccpp platform"
#endif

#define __CCPP_ARCH_PATH(path)          <ccpp/bits/arch/__CCPP_ARCH/path>
#define __CCPP_PLATFORM_PATH(path)      <ccpp/bits/__CCPP_PLATFORM/path>
#define __CCPP_ARCH_PLATFORM_PATH(path) <ccpp/bits/arch/__CCPP_ARCH/__CCPP_PLATFORM/path>
