#include "../../csc.hpp"

using namespace csc;

int main(int argc, char** argv) {
    Cmd cmd("clang++", "-o", "main", "main.cpp");
    cmd.Append("-std=c++23");
    run_cmd(cmd);
    // log(INFO, "have success build target!");
}
