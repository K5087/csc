#include "../../csc.hpp"

using namespace csc;
using namespace csc::ToolChain;

void compile_module() {
    Clang clang("clang++");

    // Compiler<CompilerType::clang> clang;
    build::compile_translation_unit(clang, clang.compile_module_option(Path("answer.cppm")));

    // Cmd stdcmd;
    // stdcmd.Append("clang++", "-std=c++23", "-stdlib=libc++", "-Wno-reserved-module-identifier", "--precompile", "-o", "std.pcm",
    //            "C:/OS/ToolChain/llvm-mingw-20251216-ucrt-x86_64/share/libc++/v1/std.cppm");
    // run_cmd(stdcmd);
}

void build_target() {
    Cmd cmd;
    cmd.Append("clang++", "-std=c++23", "-o", "main", "main.cpp", "-fmodule-file=answer=answer.pcm",
               "answer.pcm");
    run_cmd(cmd);
}

int main(int argc, char* argv[]) {
    auto result = build::update_self(argc, argv, __FILE__);
    if (!result) {
        log(ERRO, result.error());
    } else {
        if (result.value()) return 0;
    }
    compile_module();
    build_target();
}
