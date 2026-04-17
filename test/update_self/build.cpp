#include "../../csc.hpp"

using namespace csc;

int main(int argc, char** argv) {
    update_self(argc, argv, __FILE__, {"../../csc.hpp"});

    CMD("clang++", "-o", "main", "main.cpp", "-std=c++23");
}
