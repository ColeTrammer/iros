#include "params.h"

#include "di/container/view/transform.h"
#include "di/format/prelude.h"
#include "di/parser/prelude.h"

namespace ttx {
auto Params::from_string(di::StringView view) -> Params {
    auto params = view | di::split(U';') | di::transform([](di::StringView nums) -> di::Vector<u32> {
                      return nums | di::split(U':') | di::transform([](di::StringView num) -> u32 {
                                 // Invalid or empty parameters get defaulted to 1, which is suitable for
                                 // parsing input codes.
                                 return di::parse<i32>(num).value_or(1);
                             }) |
                             di::to<di::Vector>();
                  }) |
                  di::to<di::Vector>();
    return Params(di::move(params));
}

auto Subparams::to_string() const -> di::String {
    return m_subparams | di::transform(di::to_string) | di::join_with(U':') | di::to<di::String>();
}

auto Params::to_string() const -> di::String {
    return m_parameters | di::transform([&](auto const& subparams) {
               return Subparams(subparams.span()).to_string();
           }) |
           di::join_with(U';') | di::to<di::String>();
}
}
