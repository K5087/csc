#include <argp/argp.h>
#include <log/log.h>
#include <rsc/rsc.h>

using namespace csc;

void handle_run(std::string_view input, std::string_view name = "") {
    Path source = input;
    Path dir;
    if (name.empty()) {
        dir = rsc::script / source.stem();
    } else {
        dir = rsc::script / name;
    }
    if (!std::filesystem::exists(dir)) {
        std::filesystem::create_directory(dir);
    }
    auto output = Path(input).replace_extension(get_extension()).generic_string();

    cmd::Cmd compile{get_compiler(), input, "-o", output};
    if (!csc::update_bin(input, {source}, compile)) {
        return;
    }

    cmd::Cmd exec{output};
    if (cmd::run_cmd(exec).value.value_or(-1) != 0) {
        loge("execute %s failed", output.c_str());
    }
}

void handle_arg(int argc, char** argv) {
    argp::Parser parser;
    parser.add_opt({"-h", "--help"}, "print helper", argp::Boundary::get_self);
    parser.add_opt({"-v", "--version"}, "get version", argp::Boundary::get_self);
    parser.add_opt({"-n", "--name"}, "named build,excape explicit name", argp::Boundary::get_self);

    parser.add_pos("input", false, "input script file path", argp::Boundary::another_rule);
    parser.parse(argc, argv);

    if (!parser.get_args("-h").empty()) {
        parser.print_helper("rsc");
        return;
    }
    if (!parser.get_args("-v").empty()) {
        printf("rsc 1.0.0 build by moke");
        return;
    }

    std::string_view name;

    auto names = parser.get_args("-n");

    name = names.empty() ? "" : names.front();

    auto inputs = parser.get_pos(0);
    if (!inputs.empty()) {
        handle_run(inputs[0], name);
    }
}

int main(int argc, char* argv[]) {
    // default install binary as current path
    rsc::init();
    handle_arg(argc, argv);

    return 0;
}
