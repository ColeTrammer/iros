#pragma once

namespace di::types {
template<template<typename...> typename Template>
struct InPlaceTemplate {};

template<template<typename...> typename Template>
constexpr inline auto in_place_template = InPlaceTemplate<Template> {};
}
