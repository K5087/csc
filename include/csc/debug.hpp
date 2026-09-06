#pragma once
#include <csc/csc.h>
#include <iostream>

inline void print_cmd(const cmd::Cmd& cmd) {
    bool is_first = true;
    for (auto& arg : cmd) {
        if (is_first) {
            is_first = false;
        } else {
            std::cout << " ";
        }
        std::cout << arg;
    }
    std::cout << "\n";
}
