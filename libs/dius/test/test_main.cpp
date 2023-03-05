#include <dius/test/prelude.h>

namespace dius::test {
static di::Result<void> main(TestManager::Args& args) {
    auto result = dius::test::TestManager::the().run_tests(args);
#ifdef DIUS_PLATFORM_IROS
    TRY(system::system_call<int>(system::Number::shutdown, !result.has_value()));
#endif
    return result;
}
}

DIUS_MAIN(dius::test::TestManager::Args, dius::test)
