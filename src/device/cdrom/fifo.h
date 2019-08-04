#pragma once
#include <cstddef>

template <size_t length, typename T>
class fifo {
    // Elements are added to the back and are read from the front
    T data[length] = {};
    size_t write_ptr = 0;
    size_t read_ptr = 0;

   public:
    size_t size() const {
        if (write_ptr >= read_ptr) {
            return write_ptr - read_ptr;
        } else {
            return length + write_ptr - read_ptr;
        }
    }

    bool empty() const { return size() == 0; }

    bool full() const { return size() == length - 1; }

    void clear() {
        write_ptr = 0;
        read_ptr = 0;
    }

    bool add(const T t) {
        if (full()) {
            return false;
        }

        data[write_ptr] = t;
        write_ptr = ++write_ptr % length;

        return true;
    }

    T get() {
        if (empty()) {
            return 0;
            // TODO ?
        }

        T t = data[read_ptr];
        read_ptr = ++read_ptr % length;

        return t;
    }

    T peek(const size_t ptr = 0) const {
        if (ptr >= size()) {
            return 0;
            // TODO ?
        }

        return data[(read_ptr + ptr) % length];
    }

    T operator[](T ptr) const { return peek(ptr); }
};