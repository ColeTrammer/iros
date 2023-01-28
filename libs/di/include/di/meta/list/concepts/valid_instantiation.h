#pragma once

namespace di::concepts {
template<template<typename...> typename Fun, typename... Args>
concept ValidInstantiation = requires { typename Fun<Args...>; };
}