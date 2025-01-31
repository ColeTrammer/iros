#include "ttx/params.h"

#include "di/container/view/transform.h"
#include "di/format/prelude.h"
#include "di/parser/prelude.h"

namespace ttx {
auto Params::from_string(di::StringView view) -> Params {
    auto params = view | di::split(U';') | di::transform([](di::StringView nums) -> di::Vector<Param> {
                      return nums | di::split(U':') | di::transform([](di::StringView num) -> Param {
                                 return di::parse<u32>(num)
                                     .transform([](u32 value) {
                                         return Param(value);
                                     })
                                     .value_or(Param());
                             }) |
                             di::to<di::Vector>();
                  }) |
                  di::to<di::Vector>();
    return Params(di::move(params));
}

auto Subparams::to_string() const -> di::String {
    return m_subparams | di::transform([](Param param) -> di::String {
               if (!param.has_value()) {
                   return {};
               }
               return di::to_string(param.value());
           }) |
           di::join_with(U':') | di::to<di::String>();
}

auto Params::to_string() const -> di::String {
    return m_parameters | di::transform([&](auto const& subparams) {
               return Subparams(subparams.span()).to_string();
           }) |
           di::join_with(U';') | di::to<di::String>();
}
}
