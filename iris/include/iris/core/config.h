#pragma once

#ifdef __x86_64__
// clang-format off
#define IRIS_ARCH x86/amd64
// clang-format on

#define IRIS_ARCH_AMD64
#else
#error "Unsupported architecture for Iris kernel"
#endif

#define IRIS_ARCH_INCLUDE(path) <iris/arch/IRIS_ARCH/path>
