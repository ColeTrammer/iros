#include <liim/error.h>
#include <test/test.h>

using LIIM::Error;
using LIIM::ErrorDomain;
using LIIM::ErrorTransport;

class StringErrorDomain : public ErrorDomain {
public:
    using ErrorType = UniquePtr<String>;

    // virtual void destroy_error(ErrorTransport<>& value) { destroy_error(__builtin_bitcast(value, ErrorTransport<ErrorType>)); }
    // virtual StringView message(const ErrorTransport<>& value) { message(__builtin_bitcast(value, ErrorTransport<ErrorType>)); }
    virtual StringView type() const { return "StringError"sv; }

private:
};

TEST(error, basic) {}
