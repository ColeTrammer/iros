#pragma once

namespace LIIM {

class String;

}

class Panel {
public:
    virtual ~Panel() {}

    virtual int rows() const = 0;
    virtual int cols() const = 0;

    virtual void clear() = 0;
    virtual void set_text_at(int row, int col, char c) = 0;
    virtual void flush() = 0;
    virtual void set_cursor(int row, int col) = 0;

protected:
    Panel() {}
};