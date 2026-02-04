#include "../../csc.hpp"

using namespace csc;
using namespace csc::ToolChain;

void update_self(int argc, char** argv) {
    auto result = build::update_self(argc, argv, __FILE__, {"../../csc.hpp"});
    if (!result) {
        log(ERRO, result.error());
    } else {
        // update success,exit old build
        if (result.value()) std::exit(0);
    }
}

void test(){
    csc::ToolChain::ToolChain mingw = find_ToolChain("const Dir &dir");
    
}
int main(int argc, char* argv[]) {
    update_self(argc, argv);

    // Target target("target");
    //
    // Unit main("main.cpp");
    // Unit answer("answer.c");
    // target.add_translation_units({main, answer});
    // target.add_options("-std=c++23");
    //
    // Clang clang;
    // bool  result = build_target(clang, target);
    //
    // if (result) {
    //     log(INFO, "build target success.");
    // } else {
    //     log(ERRO, "build target failed.");
    // }

}
