#include "../../csc.hpp"

int main(int argc, char** argv) {
    using namespace csc;
    using std::filesystem::path;

    auto result = build::update_self(argc, argv, __FILE__);
    if (!result) {
        log(ERRO, result.error());
    } else {
        if (result.value()) return 0;
    }
    Cmd cmd("clang++", "-o", "main", "main.cpp");
    cmd.Append("-std=c++23");
    run_cmd(cmd);

    log(INFO, "have success build target!");
}
