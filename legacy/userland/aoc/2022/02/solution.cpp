#include "../../aoc_problem_registry.h"

static int score(c32 opp, c32 me) {
    auto their_choice = opp - 'A';
    auto our_choice = me - 'X';
    auto outcome = our_choice == their_choice ? 1 : (their_choice + 1) % 3 == our_choice ? 2 : 0;
    return (1 + our_choice) + outcome * 3;
}

static int score2(c32 opp, c32 me) {
    auto their_choice = opp - 'A';
    auto outcome = me - 'X';
    auto our_choice = outcome == 0 ? (their_choice + 5) % 3 : outcome == 1 ? their_choice : (their_choice + 1) % 3;
    return (1 + our_choice) + outcome * 3;
}

AOC_SOLUTION(2022, 2, a, i32) {
    return input | di::split(U'\n') |
           di::transform(di::split(U' ') | di::transform([](auto x) {
                             return *x.front();
                         }) |
                         di::pairwise_transform(score)) |
           di::join | di::sum;
}

AOC_SOLUTION(2022, 2, b, i32) {
    return input | di::split(U'\n') |
           di::transform(di::split(U' ') | di::transform([](auto x) {
                             return *x.front();
                         }) |
                         di::pairwise_transform(score2)) |
           di::join | di::sum;
}