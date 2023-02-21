#pragma once

#include <di/prelude.h>
#include <dius/prelude.h>

namespace aoc {
class AocProblemRegistry {
private:
    using Function = void (*)(di::StringView);
    using Key = di::Tuple<int, int, bool>;

public:
    static inline AocProblemRegistry& the() {
        static AocProblemRegistry s_the {};
        return s_the;
    }

    constexpr void register_solver(Key key, Function function) { m_map[key] = function; }

    constexpr auto lookup(Key key) const { return m_map.at(key); }

private:
    AocProblemRegistry() = default;

    di::TreeMap<Key, Function> m_map;
};

#define AOC_SOLUTION(year, day, part, Ret)                                                                                                 \
    static Ret solve_##year##_##day##_##part(di::StringView);                                                                              \
    static __attribute__((constructor)) void __registersolve_##year##_##day##_##part() {                                                   \
        aoc::AocProblemRegistry::the().register_solver({ year, day, "" #part ""_tsv == "a"_tsv ? false : true }, [](di::StringView view) { \
            auto result = solve_##year##_##day##_##part(view);                                                                             \
            dius::println("" #year " day " #day " part " #part ": {}"_sv, result);                                                         \
        });                                                                                                                                \
    }                                                                                                                                      \
    static Ret solve_##year##_##day##_##part(di::StringView input)
}
