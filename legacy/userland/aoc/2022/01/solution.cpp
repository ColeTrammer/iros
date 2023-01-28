#include "../../aoc_problem_registry.h"

AOC_SOLUTION(2022, 1, a, i32) {
    return input | di::split(U'\n') | di::split(""_sv) | di::transform(di::transform(di::parse_unchecked<i32>) | di::sum) |
           di::to<di::PriorityQueue>() | di::take(1) | di::sum;
}

AOC_SOLUTION(2022, 1, b, i32) {
    return input | di::split(U'\n') | di::split(""_sv) | di::transform(di::transform(di::parse_unchecked<i32>) | di::sum) |
           di::to<di::PriorityQueue>() | di::take(3) | di::sum;
}