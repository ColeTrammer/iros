#include <di/any/concepts/method.h>

namespace di::meta {
template<concepts::Method Method>
using MethodTag = Method::Tag;
}