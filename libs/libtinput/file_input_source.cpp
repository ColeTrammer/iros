#include <stdlib.h>
#include <tinput/file_input_source.h>
#include <tinput/repl.h>

namespace TInput {
UniquePtr<FileInputSource> FileInputSource::create_from_path(Repl& repl, const String& path) {
    FILE* file = fopen(path.string(), "r");
    if (!file) {
        return nullptr;
    }

    return make_unique<FileInputSource>(repl, file);
}

FileInputSource::FileInputSource(Repl& repl, FILE* file) : InputSource(repl), m_file(file) {}

FileInputSource::~FileInputSource() {
    fclose(m_file);
}

InputResult FileInputSource::get_input() {
    char* line = nullptr;
    size_t line_max = 0;

    clear_input();

    ssize_t line_length;
    while ((line_length = getline(&line, &line_max, m_file)) != -1) {
        char* trailing_newline = strrchr(line, '\n');
        if (trailing_newline) {
            *trailing_newline = '\0';
        }

        auto input = input_text();
        if (!input.empty()) {
            input += String("\n");
        }
        input += String(line);
        set_input(input);

        auto status = repl().get_input_status(input_text());
        if (status == InputStatus::Finished) {
            free(line);
            return InputResult::Success;
        }
    }

    free(line);
    if (!input_text().empty()) {
        return InputResult::Success;
    }
    return InputResult::Eof;
}
}
