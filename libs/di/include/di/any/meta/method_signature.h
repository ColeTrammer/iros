#include <di/any/concepts/method.h>

namespace di::meta {
template<concepts::Method Method>
using MethodSignature = Method::Signature;
}