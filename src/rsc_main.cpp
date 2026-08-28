#include <argp/argp.h>
#include <csc/debug.hpp>
#include <csc/target.h>
#include <csc/tool_chain.h>
#include <log/log.h>
#include <rsc/rsc.h>

using namespace csc;
namespace fs = std::filesystem;

/* have path mean will generate in path, name will not use*/
void handle_run(std::string_view input, std::string_view name,
                const Path& path = {}, bool link = false,
                bool use_static = false) {
    Path source = input;
    Path dir;
    std::string output;
    if (!path.empty()) {
        if (fs::is_directory(path)) {
            dir = path;
        } else {
            dir = path.parent_path();
            output = path.generic_string();
        }
    } else if (name.empty()) {
        dir = rsc::get::script_dir() / source.stem();
    } else {
        dir = rsc::get::script_dir() / name;
    }

    if (output.empty()) {
        output = (dir / source.filename())
                     .replace_extension(get_extension())
                     .generic_string();
    }

    std::filesystem::create_directory(dir);
    fs::path package_dir = rsc::get::package_dir("rsc");
    std::string csc =
        (package_dir / ("bin/csc" + std::string(get_extension()))).string();
    std::string rsc =
        (package_dir /
         ("lib/static/rsc" + std::string(get_extension(TargetType::share))))
            .string();
    cmd::Cmd compile{csc, input, "-o", output, "-L", rsc};
    auto Ret = cmd::run_cmd(compile);
    if (Ret.value.value_or(-1) != 0) { return; }
    cmd::Cmd execute = {output};
    cmd::run_cmd(execute);
}

void handle_arg(int argc, char** argv) {
    argp::Parser parser;
    parser.add_opt({"-h", "--help"}, "print helper", argp::Boundary::get_self);
    parser.add_opt({"-v", "--version"}, "get version",
                   argp::Boundary::get_self);

    parser.add_opt({"-n", "--name"}, "named build,excape explicit name",
                   argp::Boundary::get_self);
    parser.add_opt({"-l", "--link"}, "link rsc lib, (default is shared lib)",
                   argp::Boundary::get_self);
    parser.add_opt({"-s", "--static"},
                   "link target with static lib,must have -l arg",
                   argp::Boundary::get_self);
    parser.add_opt({"-o", "--output"}, "output exec file at dir or path",
                   argp::Boundary::one_arg);

    parser.add_pos("input", false, "input script file path",
                   argp::Boundary::another_rule);

    parser.parse(argc, argv);

    /* help */
    if (!parser.get_args("-h").empty()) {
        parser.print_helper("rsc");
        return;
    }

    /* version */
    if (!parser.get_args("-v").empty()) {
        printf("rsc 1.0.0 build by moke");
        return;
    }

    std::string_view name = parser.get_arg("-n", "");
    bool use_link = !parser.get_args("-l").empty();
    bool use_static = !parser.get_args("-s").empty();
    Path output = parser.get_arg("-o", "");

    auto inputs = parser.get_pos(0);
    if (inputs.empty()) {
        loge("no input file");
        return;
    } else {
        handle_run(inputs[0], name, output, use_link, use_static);
    }
}

int main(int argc, char* argv[]) {
    // default install binary as current path
    rsc::init();
    handle_arg(argc, argv);

    return 0;
}
