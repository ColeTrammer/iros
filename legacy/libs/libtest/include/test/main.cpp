#include <cli/cli.h>
#include <test/test_manager.h>

constexpr auto parser = [] {
    using Arguments = Test::TestManager::Arguments;
    using namespace Cli;
    return Parser::of<Arguments>()
        .flag(Flag::boolean<&Arguments::list_simple>()
                  .short_name('L')
                  .long_name("list-simple")
                  .description("Output a simple machine readable list of test cases"))
        .flag(Flag::optional<&Arguments::suite_name>().short_name('s').long_name("suite").description("Specifc test suite to run"))
        .flag(Flag::optional<&Arguments::case_name>()
                  .short_name('t')
                  .long_name("test-case")
                  .description("Specific case to run in the format ([suite:]case)"));
}();

CLI_MAIN(Test::TestManager::the().do_main, parser);
