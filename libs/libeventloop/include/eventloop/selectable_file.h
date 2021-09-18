#pragma once

#include <eventloop/selectable.h>
#include <liim/function.h>
#include <liim/string.h>
#include <sys/types.h>

namespace App {

class SelectableFile : public Selectable {
    APP_OBJECT(SelectableFile)

public:
    SelectableFile(const String& path, int oflags, mode_t mode = 0);
    virtual ~SelectableFile() override;
};

}
