#include "../../csc.hpp"

using namespace csc;
using namespace csc::ToolChain;

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
    update_self(argc, argv, __FILE__, {"../../csc.hpp"});
    compile_module();
    build_target();
}
