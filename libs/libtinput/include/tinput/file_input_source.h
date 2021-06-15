#pragma once

#include <stdio.h>
#include <tinput/input_source.h>

namespace TInput {
class FileInputSource final : public InputSource {
public:
    static UniquePtr<FileInputSource> create_from_path(Repl& repl, const String& path);

    FileInputSource(Repl& repl, FILE* file);
    virtual ~FileInputSource() override;

    virtual InputResult get_input() override;

private:
    FILE* m_file;
};
}
