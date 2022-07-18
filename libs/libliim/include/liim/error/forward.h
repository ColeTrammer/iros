#pragma once

namespace LIIM::Error {
class ErrorDomain;

template<typename T = void>
struct ErrorTransport;

template<typename T = void, typename Domain = ErrorDomain>
class Error;
}
