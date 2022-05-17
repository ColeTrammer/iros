#include <cli/cli.h>
#include <errno.h>
#include <ext/parse_mode.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static Result<Monostate, Ext::StringError> mkfifo(const String& file, mode_t mode) {
    if (mkfifo(file.string(), mode)) {
        return Err(Ext::StringError(format("Failed to create `{}': {}", file, strerror(errno))));
    }
    return Ok(Monostate {});
}

struct Arguments {
    Option<Ext::Mode> mode;
    Vector<String> files;
};

static auto mkfifo_main(Arguments arguments) {
    mode_t umask_value = umask(0);
    mode_t mode = 0666 & ~umask_value;
    if (arguments.mode) {
        mode = arguments.mode->resolve(mode, umask_value);
    }

    return Ext::collect_errors(arguments.files, [&](auto& file) {
        return mkfifo(file, mode);
    });
}

CLI_MAIN(mkfifo_main, [] {
    using namespace Cli;
    return Parser::of<Arguments>()
        .flag(Flag::optional<&Arguments::mode>().short_name('m').long_name("mode").description("Mode to create fifos with"))
        .argument(Argument::list<&Arguments::files>("files").description("Fifos to create"));
}())
