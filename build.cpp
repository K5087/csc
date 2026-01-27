#include "csc.hpp"

int main(int argc, char** argv) {
    using namespace csc;
    using std::filesystem::path;

    auto result = build::register_rebuild_self(argc, argv, "build.cpp");
    if (!result) {
        csc_log(CSC_ERRO, result.error().message);
    } else {
        if (result.value()) return 0;
    }
    Cmd cmd;
    cmd.Append("clang++", "-o", "main", "main.cpp");
    csc::run_cmd(cmd);
    csc_log(CSC_INFO, "have success build target!");
}
