#pragma once

struct MouseEvent {
    enum Press {
        Down,
        Double,
        Triple,
        None,
        Up,
    };

    enum Button {
        Left = 1,
        Middle = 2,
        Right = 4,
    };

    Press left;
    Press right;
    int down;
    int index_into_line;
    int index_of_line;
    int z;
};
