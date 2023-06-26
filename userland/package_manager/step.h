#pragma once

#include <di/function/container/function.h>
#include <di/reflect/prelude.h>

#include "config.h"

namespace pm {
class Package;

enum class StepKind {
    Download,
    Patch,
    Configure,
    Build,
    Install,
    Clean,
};

constexpr auto tag_invoke(di::Tag<di::reflect>, di::InPlaceType<StepKind>) {
    using enum StepKind;
    return di::make_enumerators(di::enumerator<"download", Download>, di::enumerator<"patch", Patch>,
                                di::enumerator<"configure", Configure>, di::enumerator<"build", Build>,
                                di::enumerator<"install", Install>, di::enumerator<"clean", Clean>);
}

class Step {
public:
    explicit Step(StepKind kind, di::Function<di::Result<>(Config const&, Package&)> action)
        : m_kind(kind), m_action(di::move(action)) {}

    auto kind() const { return m_kind; }
    auto run(Config const& config, Package& package) { return m_action(config, package); }

private:
    StepKind m_kind;
    di::Function<di::Result<>(Config const&, Package&)> m_action;
};
}
