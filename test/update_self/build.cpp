#include "../../csc.hpp"

using namespace csc;

void update_self(int argc, char** argv) {
    auto result = build::update_self(argc, argv, __FILE__,{"../../csc.hpp"});

    if (!result) {
        log(ERRO, result.error());
    } else {
        // update success,exit old build
        if (result.value()) std::exit(0);
    }
}

int main(int argc, char** argv) {
    update_self(argc, argv);

    Cmd cmd("clang++", "-o", "main", "main.cpp");
    cmd.Append("-std=c++23");
    run_cmd(cmd);

    // log(INFO, "have success build target!");
}
