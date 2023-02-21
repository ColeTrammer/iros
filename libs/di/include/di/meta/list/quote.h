#pragma once

#include <di/meta/list/defer.h>

namespace di::meta {
template<template<typename...> typename Fun>
struct Quote {
    template<typename... Args>
    using Invoke = typename Defer<Fun, Args...>::Type;
};
}
