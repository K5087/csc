#include "csc.hpp"

int main(int argc, char** argv) {
    using namespace csc;
    Cmd cmd;
    try {
        cmd.Append("clang++", "-o", "main", "main.cpp");
        csc::run_cmd(cmd);

    } catch (const std::exception& e) {
        csc_log(CSC_ERRO, e.what());
        return 1;
    }
}
