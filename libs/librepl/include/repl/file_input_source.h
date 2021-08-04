#pragma once

#include <repl/input_source.h>
#include <stdio.h>

namespace Repl {
class FileInputSource final : public InputSource {
public:
    static UniquePtr<FileInputSource> create_from_path(ReplBase& repl, const String& path);

    FileInputSource(ReplBase& repl, FILE* file);
    virtual ~FileInputSource() override;

    virtual InputResult get_input() override;

private:
    FILE* m_file;
};
}
