#pragma once

#ifdef __x86_64__
#define DIUS_ARCH x86_64
#define DIUS_ARCH_X86_64
#else
#error "Unsupported dius architecture"
#endif

#ifdef __linux__
// Why does linux define linux to 1? Anyway, let's undefine it, since it causes lots of problems.
#undef linux

#define DIUS_PLATFORM linux
#define DIUS_PLATFORM_LINUX
#elif defined(__iros__)
#define DIUS_PLATFORM iros
#define DIUS_PLATFORM_IROS
#else
#error "Unsupported dius platform"
#endif

#define DIUS_ARCH_PATH(path)          <dius/arch/DIUS_ARCH/path>
#define DIUS_PLATFORM_PATH(path)      <dius/DIUS_PLATFORM/path>
#define DIUS_ARCH_PLATFORM_PATH(path) <dius/arch/DIUS_ARCH/DIUS_PLATFORM/path>
