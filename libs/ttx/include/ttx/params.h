#pragma once

#include "di/container/interface/access.h"
#include "di/container/string/string.h"
#include "di/container/string/string_view.h"
#include "di/container/vector/vector.h"
#include "di/util/initializer_list.h"

namespace ttx {
// Paramaters are integer with the addition of nullability.
// We use a special type so that we be more space efficent.
class Param {
public:
    Param() = default;

    constexpr Param(u32 value) : m_value(value + 1) {}

    constexpr auto value() const -> u32 {
        DI_ASSERT(has_value());
        return m_value - 1;
    }

    constexpr auto has_value() const -> bool { return m_value != 0; }

    constexpr auto value_or(u32 fallback) const -> u32 {
        if (!has_value()) {
            return fallback;
        }
        return value();
    }

    auto operator==(Param const&) const -> bool = default;

private:
    u32 m_value { 0 };
};

// Subparams are separated by `:` characters. The subparam
// object implicitly holds a reference to its corresponding
// `Params` class.
class Subparams {
public:
    Subparams() = default;

    constexpr auto get(usize index = 0, u32 fallback = 0) const -> u32 {
        return m_subparams.at(index).value_or(Param(fallback)).value_or(fallback);
    }

    constexpr auto empty() const { return m_subparams.empty(); }
    constexpr auto size() const { return m_subparams.size(); }

    auto to_string() const -> di::String;

    auto operator==(Subparams const& other) const -> bool = default;

private:
    friend class Params;

    constexpr explicit Subparams(di::Span<Param const> subparams) : m_subparams(subparams) {}

    di::Span<Param const> m_subparams;
};

// Represents a series of numeric parameters for an escape
// sequence. Parameters are separated by `;` characeters, and
// subparameters are separated by `:` characters.
class Params {
public:
    static auto from_string(di::StringView view) -> Params;

    Params() = default;

    constexpr Params(std::initializer_list<std::initializer_list<Param>> params) {
        for (auto const& subparams : params) {
            m_parameters.emplace_back(subparams);
        }
    }

    constexpr auto clone() const -> Params { return Params(m_parameters.clone()); }

    constexpr auto get(usize index = 0, u32 fallback = 0) const -> u32 {
        return m_parameters.at(index).and_then(di::at(0)).value_or(Param(fallback)).value_or(fallback);
    }
    constexpr auto get_subparam(usize index = 0, usize subindex = 1, u32 fallback = 0) const -> u32 {
        return m_parameters.at(index).and_then(di::at(subindex)).value_or(Param(fallback)).value_or(fallback);
    }

    constexpr auto empty() const { return m_parameters.empty(); }
    constexpr auto size() const { return m_parameters.size(); }

    constexpr auto subparams(usize index = 0) const -> Subparams {
        auto span = m_parameters.at(index)
                        .transform([&](auto const& subparams) {
                            return subparams.span();
                        })
                        .value_or(di::Span<Param const> {});
        return Subparams(span);
    }

    constexpr void add_empty_param() { m_parameters.emplace_back(); }
    constexpr void add_param(u32 value) { m_parameters.push_back({ value }); }

    constexpr void add_subparam(u32 value) {
        if (empty()) {
            add_param(value);
        } else {
            m_parameters.back().value().push_back(value);
        }
    }
    constexpr void add_empty_subparam() {
        if (empty()) {
            add_empty_param();
        }
        m_parameters.back().value().emplace_back();
    }
    constexpr void add_subparams(di::Vector<Param> subparams) { m_parameters.push_back(di::move(subparams)); }

    auto to_string() const -> di::String;

    auto operator==(Params const& other) const -> bool = default;

private:
    constexpr explicit Params(di::Vector<di::Vector<Param>> params) : m_parameters(di::move(params)) {}

    di::Vector<di::Vector<Param>> m_parameters;
};
}
