#include "../../csc.hpp"

using namespace csc;
using namespace csc::ToolChain;

void update_self(int argc, char** argv) {
    auto result = build::update_self(argc, argv, __FILE__,{"../../csc.hpp"});
    if (!result) {
        log(ERRO, result.error());
    } else {
        // update success,exit old build
        if (result.value()) std::exit(0);
    }
}

void compile_module() {
    Clang clang;
    Cmd   cmd(clang.path, clang.compile_module_option("answer.cppm"), "-std=c++23");
    run_cmd(cmd);
}

void build_target() {
    Cmd cmd;
    cmd.Append("clang++", "-std=c++23", "-o", "main", "main.cpp", "-fmodule-file=answer=answer.pcm",
               "answer.pcm");
    run_cmd(cmd);
}

int main(int argc, char* argv[]) {
    update_self(argc, argv);
    compile_module();
    build_target();
}
