#include <ipc/gen.h>

// clang-format off
IPC_MESSAGES(Test,
    (M1,
        (int, z), 
        (int, x),
        (String, s)
    )
)
// clang-format on

int main() {
    Test::M1 x = { .z = 1, .x = 2, .s = "asdf" };

    char buffer[400];
    IPC::Stream istream(buffer, sizeof(buffer));
    IPC::Stream ostream(buffer, sizeof(buffer));

    assert(x.serialize(ostream));

    Test::M1 y;
    assert(y.deserialize(istream));

    assert(y.z == 1);
    assert(y.x == 2);
    assert(y.s == "asdf");
}
