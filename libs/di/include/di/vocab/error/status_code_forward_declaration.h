#pragma once

namespace di::platform {
class GenericDomain;
}

namespace di::vocab {
class StatusCodeDomain;

template<typename Domain>
class StatusCode;

using GenericCode = StatusCode<platform::GenericDomain>;
}
