#include <dius/test/prelude.h>

namespace dius::test {
static di::Result<void> main(TestManager::Args& args) {
    return dius::test::TestManager::the().run_tests(args);
}
}

DIUS_MAIN(dius::test::TestManager::Args, dius::test)
