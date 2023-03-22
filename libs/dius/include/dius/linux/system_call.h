#pragma once

#include <asm/unistd.h>
#include <linux/fcntl.h>
#include <linux/io_uring.h>
#include <linux/mman.h>

#include <di/prelude.h>
#include <dius/config.h>
#include <dius/error.h>

#include DIUS_ARCH_PLATFORM_PATH(system_call.h)

namespace dius::system {
enum class Number : int {
    io_uring_enter = __NR_io_uring_enter,
    io_uring_setup = __NR_io_uring_setup,
    io_uring_register = __NR_io_uring_register,
    pread = __NR_pread64,
    pwrite = __NR_pwrite64,
    read = __NR_read,
    write = __NR_write,
    close = __NR_close,
    openat = __NR_openat,
    mmap = __NR_mmap,
    munmap = __NR_munmap,
    getdents64 = __NR_getdents64,
    fstatat64 = __NR_newfstatat,
    ftruncate = __NR_ftruncate,
    arch_prctl = __NR_arch_prctl,
    brk = __NR_brk,
    exit_group = __NR_exit_group,
    clone3 = __NR_clone3,
    execve = __NR_execve,
    wait4 = __NR_wait4
};

using SystemCallArg = unsigned long;
using SystemCallResult = long;

namespace detail {
    template<typename T>
    concept SystemCallArgument = di::concepts::ConstructibleFrom<SystemCallArg, T>;

    template<typename T>
    concept SystemCallResult = di::concepts::ConstructibleFrom<T, SystemCallResult>;
}

template<detail::SystemCallResult R>
di::Expected<R, dius::PosixError> system_call(Number number) {
    SystemCallResult res;
    asm volatile(DIUS_SYSTEM_CALL_INSTRUCTION
                 : DIUS_SYSTEM_CALL_ASM_RESULT(res)
                 : DIUS_SYSTEM_CALL_ASM_NUMBER(di::to_underlying(number))
                 : DIUS_SYSTEM_CALL_CLOBBER);
    if (res < 0) {
        return di::Unexpected(dius::PosixError(-res));
    }
    return R(res);
}

template<detail::SystemCallResult R, detail::SystemCallArgument T1>
di::Expected<R, dius::PosixError> system_call(Number number, T1&& a1) {
    SystemCallResult res;
    SystemCallArg y1 = SystemCallArg(a1);
    register SystemCallArg x1 asm(DIUS_SYSTEM_CALL_ASM_ARG1) = y1;
    asm volatile(DIUS_SYSTEM_CALL_INSTRUCTION
                 : DIUS_SYSTEM_CALL_ASM_RESULT(res)
                 : DIUS_SYSTEM_CALL_ASM_NUMBER(di::to_underlying(number)), "r"(x1)
                 : DIUS_SYSTEM_CALL_CLOBBER);
    if (res < 0) {
        return di::Unexpected(dius::PosixError(-res));
    }
    return R(res);
}

template<detail::SystemCallResult R, detail::SystemCallArgument T1, detail::SystemCallArgument T2>
di::Expected<R, dius::PosixError> system_call(Number number, T1&& a1, T2&& a2) {
    SystemCallResult res;
    SystemCallArg y1 = SystemCallArg(a1);
    SystemCallArg y2 = SystemCallArg(a2);
    register SystemCallArg x1 asm(DIUS_SYSTEM_CALL_ASM_ARG1) = y1;
    register SystemCallArg x2 asm(DIUS_SYSTEM_CALL_ASM_ARG2) = y2;
    asm volatile(DIUS_SYSTEM_CALL_INSTRUCTION
                 : DIUS_SYSTEM_CALL_ASM_RESULT(res)
                 : DIUS_SYSTEM_CALL_ASM_NUMBER(di::to_underlying(number)), "r"(x1), "r"(x2)
                 : DIUS_SYSTEM_CALL_CLOBBER);
    if (res < 0) {
        return di::Unexpected(dius::PosixError(-res));
    }
    return R(res);
}

template<detail::SystemCallResult R, detail::SystemCallArgument T1, detail::SystemCallArgument T2,
         detail::SystemCallArgument T3>
di::Expected<R, dius::PosixError> system_call(Number number, T1&& a1, T2&& a2, T3&& a3) {
    SystemCallResult res;
    SystemCallArg y1 = SystemCallArg(a1);
    SystemCallArg y2 = SystemCallArg(a2);
    SystemCallArg y3 = SystemCallArg(a3);
    register SystemCallArg x1 asm(DIUS_SYSTEM_CALL_ASM_ARG1) = y1;
    register SystemCallArg x2 asm(DIUS_SYSTEM_CALL_ASM_ARG2) = y2;
    register SystemCallArg x3 asm(DIUS_SYSTEM_CALL_ASM_ARG3) = y3;
    asm volatile(DIUS_SYSTEM_CALL_INSTRUCTION
                 : DIUS_SYSTEM_CALL_ASM_RESULT(res)
                 : DIUS_SYSTEM_CALL_ASM_NUMBER(di::to_underlying(number)), "r"(x1), "r"(x2), "r"(x3)
                 : DIUS_SYSTEM_CALL_CLOBBER);
    if (res < 0) {
        return di::Unexpected(dius::PosixError(-res));
    }
    return R(res);
}

template<detail::SystemCallResult R, detail::SystemCallArgument T1, detail::SystemCallArgument T2,
         detail::SystemCallArgument T3, detail::SystemCallArgument T4>
di::Expected<R, dius::PosixError> system_call(Number number, T1&& a1, T2&& a2, T3&& a3, T4&& a4) {
    SystemCallResult res;
    SystemCallArg y1 = SystemCallArg(a1);
    SystemCallArg y2 = SystemCallArg(a2);
    SystemCallArg y3 = SystemCallArg(a3);
    SystemCallArg y4 = SystemCallArg(a4);
    register SystemCallArg x1 asm(DIUS_SYSTEM_CALL_ASM_ARG1) = y1;
    register SystemCallArg x2 asm(DIUS_SYSTEM_CALL_ASM_ARG2) = y2;
    register SystemCallArg x3 asm(DIUS_SYSTEM_CALL_ASM_ARG3) = y3;
    register SystemCallArg x4 asm(DIUS_SYSTEM_CALL_ASM_ARG4) = y4;
    asm volatile(DIUS_SYSTEM_CALL_INSTRUCTION
                 : DIUS_SYSTEM_CALL_ASM_RESULT(res)
                 : DIUS_SYSTEM_CALL_ASM_NUMBER(di::to_underlying(number)), "r"(x1), "r"(x2), "r"(x3), "r"(x4)
                 : DIUS_SYSTEM_CALL_CLOBBER);
    if (res < 0) {
        return di::Unexpected(dius::PosixError(-res));
    }
    return R(res);
}

template<detail::SystemCallResult R, detail::SystemCallArgument T1, detail::SystemCallArgument T2,
         detail::SystemCallArgument T3, detail::SystemCallArgument T4, detail::SystemCallArgument T5>
di::Expected<R, dius::PosixError> system_call(Number number, T1&& a1, T2&& a2, T3&& a3, T4&& a4, T5&& a5) {
    SystemCallResult res;
    SystemCallArg y1 = SystemCallArg(a1);
    SystemCallArg y2 = SystemCallArg(a2);
    SystemCallArg y3 = SystemCallArg(a3);
    SystemCallArg y4 = SystemCallArg(a4);
    SystemCallArg y5 = SystemCallArg(a5);
    register SystemCallArg x1 asm(DIUS_SYSTEM_CALL_ASM_ARG1) = y1;
    register SystemCallArg x2 asm(DIUS_SYSTEM_CALL_ASM_ARG2) = y2;
    register SystemCallArg x3 asm(DIUS_SYSTEM_CALL_ASM_ARG3) = y3;
    register SystemCallArg x4 asm(DIUS_SYSTEM_CALL_ASM_ARG4) = y4;
    register SystemCallArg x5 asm(DIUS_SYSTEM_CALL_ASM_ARG5) = y5;
    asm volatile(DIUS_SYSTEM_CALL_INSTRUCTION
                 : DIUS_SYSTEM_CALL_ASM_RESULT(res)
                 : DIUS_SYSTEM_CALL_ASM_NUMBER(di::to_underlying(number)), "r"(x1), "r"(x2), "r"(x3), "r"(x4), "r"(x5)
                 : DIUS_SYSTEM_CALL_CLOBBER);
    if (res < 0) {
        return di::Unexpected(dius::PosixError(-res));
    }
    return R(res);
}

template<detail::SystemCallResult R, detail::SystemCallArgument T1, detail::SystemCallArgument T2,
         detail::SystemCallArgument T3, detail::SystemCallArgument T4, detail::SystemCallArgument T5,
         detail::SystemCallArgument T6>
di::Expected<R, dius::PosixError> system_call(Number number, T1&& a1, T2&& a2, T3&& a3, T4&& a4, T5&& a5, T6&& a6) {
    SystemCallResult res;
    SystemCallArg y1 = SystemCallArg(a1);
    SystemCallArg y2 = SystemCallArg(a2);
    SystemCallArg y3 = SystemCallArg(a3);
    SystemCallArg y4 = SystemCallArg(a4);
    SystemCallArg y5 = SystemCallArg(a5);
    SystemCallArg y6 = SystemCallArg(a6);
    register SystemCallArg x1 asm(DIUS_SYSTEM_CALL_ASM_ARG1) = y1;
    register SystemCallArg x2 asm(DIUS_SYSTEM_CALL_ASM_ARG2) = y2;
    register SystemCallArg x3 asm(DIUS_SYSTEM_CALL_ASM_ARG3) = y3;
    register SystemCallArg x4 asm(DIUS_SYSTEM_CALL_ASM_ARG4) = y4;
    register SystemCallArg x5 asm(DIUS_SYSTEM_CALL_ASM_ARG5) = y5;
    register SystemCallArg x6 asm(DIUS_SYSTEM_CALL_ASM_ARG6) = y6;
    asm volatile(DIUS_SYSTEM_CALL_INSTRUCTION
                 : DIUS_SYSTEM_CALL_ASM_RESULT(res)
                 : DIUS_SYSTEM_CALL_ASM_NUMBER(di::to_underlying(number)), "r"(x1), "r"(x2), "r"(x3), "r"(x4), "r"(x5),
                   "r"(x6)
                 : DIUS_SYSTEM_CALL_CLOBBER);
    if (res < 0) {
        return di::Unexpected(dius::PosixError(-res));
    }
    return R(res);
}
}
