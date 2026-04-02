#include "../../csc.hpp"

using namespace csc;
using namespace csc::ToolChain;


int main(int argc, char* argv[]) {
    update_self(argc, argv,__FILE__,{"../../csc.hpp"});

    Target target("main");
    Unit   main("main.cpp");
    Unit   answer("answer.cpp");

    target.add_translation_units({main, answer});

    Clang clang;
    bool  result = build_target(clang, target);

    if (result) {
        log(INFO, "build target success");
    } else {
        log(ERRO, "build target failed");
    }
}
