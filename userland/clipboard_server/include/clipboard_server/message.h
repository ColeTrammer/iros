#pragma once

#include <ipc/gen.h>
#include <liim/pointers.h>
#include <liim/string.h>
#include <stdlib.h>

namespace ClipboardServer {

// clang-format off
IPC_MESSAGES(Client,
    (GetContentsRequest,
        (String, type),
    ),
    (SetContentsRequest,
        (String, type),
        (Vector<char>, data),
    ),
)

IPC_MESSAGES(Server,
    (GetContentsResponse,
        (Vector<char>, data),
    ),
    (SetContentsResponse,
        (bool, success),
    ),
)
// clang-format on

}
