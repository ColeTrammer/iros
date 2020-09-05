#pragma once

struct MouseEvent {
    enum Press {
        None,
        Down,
        Up,
    };

    enum Button {
        Left = 1,
        Right = 2,
    };

    Press left;
    Press right;
    int down;
    int index_into_line;
    int index_of_line;
    int z;
};
