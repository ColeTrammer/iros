#pragma once

#include <di/util/prelude.h>
#include <dius/config.h>
#include <dius/error.h>
#include <iris/uapi/syscall.h>

#include DIUS_ARCH_PLATFORM_PATH(system_call.h)

namespace dius::system {
using Number = iris::SystemCall;

using SystemCallArg = unsigned long;
using SystemCallResult = unsigned long;
using SystemCallError = unsigned long;

namespace detail {
    template<typename T>
    concept SystemCallArgument = di::concepts::ConstructibleFrom<SystemCallArg, T>;

    template<typename T>
    concept SystemCallResult = di::concepts::ConstructibleFrom<T, SystemCallResult>;
}

template<detail::SystemCallResult R>
di::Expected<R, di::BasicError> system_call(Number number) {
    SystemCallResult res = di::to_underlying(number);
    SystemCallResult err;
    asm volatile(DIUS_SYSTEM_CALL_INSTRUCTION
                 : DIUS_SYSTEM_CALL_ASM_RESULT(res), DIUS_SYSTEM_CALL_ASM_ERROR(err)
                 : DIUS_SYSTEM_CALL_ASM_NUMBER(res)
                 : DIUS_SYSTEM_CALL_CLOBBER);
    if (err) {
        return di::Unexpected(di::BasicError(err));
    }
    return R(res);
}

template<detail::SystemCallResult R, detail::SystemCallArgument T1>
di::Expected<R, di::BasicError> system_call(Number number, T1&& a1) {
    SystemCallResult res = di::to_underlying(number);
    SystemCallResult err;
    SystemCallArg y1 = SystemCallArg(a1);
    register SystemCallArg x1 asm(DIUS_SYSTEM_CALL_ASM_ARG1) = y1;
    asm volatile(DIUS_SYSTEM_CALL_INSTRUCTION
                 : DIUS_SYSTEM_CALL_ASM_RESULT(res), DIUS_SYSTEM_CALL_ASM_ERROR(err)
                 : DIUS_SYSTEM_CALL_ASM_NUMBER(res), "r"(x1)
                 : DIUS_SYSTEM_CALL_CLOBBER);
    if (err) {
        return di::Unexpected(di::BasicError(err));
    }
    return R(res);
}

template<detail::SystemCallResult R, detail::SystemCallArgument T1, detail::SystemCallArgument T2>
di::Expected<R, di::BasicError> system_call(Number number, T1&& a1, T2&& a2) {
    SystemCallResult res = di::to_underlying(number);
    SystemCallResult err;
    SystemCallArg y1 = SystemCallArg(a1);
    SystemCallArg y2 = SystemCallArg(a2);
    register SystemCallArg x1 asm(DIUS_SYSTEM_CALL_ASM_ARG1) = y1;
    register SystemCallArg x2 asm(DIUS_SYSTEM_CALL_ASM_ARG2) = y2;
    asm volatile(DIUS_SYSTEM_CALL_INSTRUCTION
                 : DIUS_SYSTEM_CALL_ASM_RESULT(res), DIUS_SYSTEM_CALL_ASM_ERROR(err)
                 : DIUS_SYSTEM_CALL_ASM_NUMBER(res), "r"(x1), "r"(x2)
                 : DIUS_SYSTEM_CALL_CLOBBER);
    if (err) {
        return di::Unexpected(di::BasicError(err));
    }
    return R(res);
}

template<detail::SystemCallResult R, detail::SystemCallArgument T1, detail::SystemCallArgument T2,
         detail::SystemCallArgument T3>
di::Expected<R, di::BasicError> system_call(Number number, T1&& a1, T2&& a2, T3&& a3) {
    SystemCallResult res = di::to_underlying(number);
    SystemCallResult err;
    SystemCallArg y1 = SystemCallArg(a1);
    SystemCallArg y2 = SystemCallArg(a2);
    SystemCallArg y3 = SystemCallArg(a3);
    register SystemCallArg x1 asm(DIUS_SYSTEM_CALL_ASM_ARG1) = y1;
    register SystemCallArg x2 asm(DIUS_SYSTEM_CALL_ASM_ARG2) = y2;
    register SystemCallArg x3 asm(DIUS_SYSTEM_CALL_ASM_ARG3) = y3;
    asm volatile(DIUS_SYSTEM_CALL_INSTRUCTION
                 : DIUS_SYSTEM_CALL_ASM_RESULT(res), DIUS_SYSTEM_CALL_ASM_ERROR(err)
                 : DIUS_SYSTEM_CALL_ASM_NUMBER(res), "r"(x1), "r"(x2), "r"(x3)
                 : DIUS_SYSTEM_CALL_CLOBBER);
    if (err) {
        return di::Unexpected(di::BasicError(err));
    }
    return R(res);
}

template<detail::SystemCallResult R, detail::SystemCallArgument T1, detail::SystemCallArgument T2,
         detail::SystemCallArgument T3, detail::SystemCallArgument T4>
di::Expected<R, di::BasicError> system_call(Number number, T1&& a1, T2&& a2, T3&& a3, T4&& a4) {
    SystemCallResult res = di::to_underlying(number);
    SystemCallResult err;
    SystemCallArg y1 = SystemCallArg(a1);
    SystemCallArg y2 = SystemCallArg(a2);
    SystemCallArg y3 = SystemCallArg(a3);
    SystemCallArg y4 = SystemCallArg(a4);
    register SystemCallArg x1 asm(DIUS_SYSTEM_CALL_ASM_ARG1) = y1;
    register SystemCallArg x2 asm(DIUS_SYSTEM_CALL_ASM_ARG2) = y2;
    register SystemCallArg x3 asm(DIUS_SYSTEM_CALL_ASM_ARG3) = y3;
    register SystemCallArg x4 asm(DIUS_SYSTEM_CALL_ASM_ARG4) = y4;
    asm volatile(DIUS_SYSTEM_CALL_INSTRUCTION
                 : DIUS_SYSTEM_CALL_ASM_RESULT(res), DIUS_SYSTEM_CALL_ASM_ERROR(err)
                 : DIUS_SYSTEM_CALL_ASM_NUMBER(res), "r"(x1), "r"(x2), "r"(x3), "r"(x4)
                 : DIUS_SYSTEM_CALL_CLOBBER);
    if (err) {
        return di::Unexpected(di::BasicError(err));
    }
    return R(res);
}

template<detail::SystemCallResult R, detail::SystemCallArgument T1, detail::SystemCallArgument T2,
         detail::SystemCallArgument T3, detail::SystemCallArgument T4, detail::SystemCallArgument T5>
di::Expected<R, di::BasicError> system_call(Number number, T1&& a1, T2&& a2, T3&& a3, T4&& a4, T5&& a5) {
    SystemCallResult res = di::to_underlying(number);
    SystemCallResult err;
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
                 : DIUS_SYSTEM_CALL_ASM_RESULT(res), DIUS_SYSTEM_CALL_ASM_ERROR(err)
                 : DIUS_SYSTEM_CALL_ASM_NUMBER(res), "r"(x1), "r"(x2), "r"(x3), "r"(x4), "r"(x5)
                 : DIUS_SYSTEM_CALL_CLOBBER);
    if (err) {
        return di::Unexpected(di::BasicError(err));
    }
    return R(res);
}

template<detail::SystemCallResult R, detail::SystemCallArgument T1, detail::SystemCallArgument T2,
         detail::SystemCallArgument T3, detail::SystemCallArgument T4, detail::SystemCallArgument T5,
         detail::SystemCallArgument T6>
di::Expected<R, di::BasicError> system_call(Number number, T1&& a1, T2&& a2, T3&& a3, T4&& a4, T5&& a5, T6&& a6) {
    SystemCallResult res = di::to_underlying(number);
    SystemCallResult err;
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
                 : DIUS_SYSTEM_CALL_ASM_RESULT(res), DIUS_SYSTEM_CALL_ASM_ERROR(err)
                 : DIUS_SYSTEM_CALL_ASM_NUMBER(res), "r"(x1), "r"(x2), "r"(x3), "r"(x4), "r"(x5), "r"(x6)
                 : DIUS_SYSTEM_CALL_CLOBBER);
    if (err) {
        return di::Unexpected(di::BasicError(err));
    }
    return R(res);
}
}
