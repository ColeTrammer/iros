#pragma once

namespace di::vocab {
class StatusCodeDomain;
class GenericDomain;

template<typename Domain>
class StatusCode;

using GenericCode = StatusCode<GenericDomain>;
}
