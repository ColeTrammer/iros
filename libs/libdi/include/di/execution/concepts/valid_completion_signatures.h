#pragma once

#include <di/concepts/instance_of.h>
#include <di/concepts/same_as.h>
#include <di/execution/types/completion_signuatures.h>
#include <di/execution/types/dependent_completion_signatures.h>
#include <di/execution/types/no_env.h>

namespace di::concepts {
template<typename Signatures, typename Env>
concept ValidCompletionSignatures = InstanceOf<Signatures, types::CompletionSignatures> ||
                                    (SameAs<Signatures, types::DependentCompletionSignatures<types::NoEnv>> &&
                                     SameAs<Env, types::NoEnv>);
}