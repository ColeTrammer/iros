#pragma once

namespace di::util::meta {
// This is a helper template to prevent C++ from deducing the type of template argument.
// This will force users/callers to specify the template types inside the <> brackets.
template<typename T>
using TypeIdentity = T;
}
