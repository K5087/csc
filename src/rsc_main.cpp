#include <argp/argp.h>
#include <csc/csc.h>
#include <csc/target.h>
#include <log/log.h>
#include <rsc/os.h>
#include <rsc/rsc.h>

using namespace csc;
namespace fs = std::filesystem;

struct Info {
    fs::path input;
    fs::path output;
    bool link = false;
    bool use_static = false;
    bool gen_data = false;
    bool debug = false;
};

/* have path mean will generate in path, name will not use*/
void handle_compile(const Info& info) {
    std::string input_file = info.input.generic_string();
    std::string output_file = info.output.generic_string();
    if (!fs::exists(info.output) || is_outdated(info.output, {info.input})) {
        fs::path rsc_dir = rsc::get::package_dir("rsc");
        std::string csc =
            (rsc_dir / ("bin/csc" + std::string(get_extension()))).string();
        std::string rsc;
        if (info.use_static) {
            rsc = (rsc_dir / ("lib/static/rsc" +
                              std::string(get_extension(TargetType::arch))))
                      .generic_string();

        } else {
            rsc = (rsc_dir / ("lib/shared/rsc" +
                              std::string(get_extension(TargetType::share))))
                      .generic_string();
        }
        auto package_dir = "-I" + rsc::get::package_dir().generic_string();
        cmd::Cmd compile{csc,  input_file, "-o", output_file,
                         "-L", rsc,        "-e", package_dir};
        if (info.link) { compile.emplace_back("-l"); }
        if (info.use_static) { compile.emplace_back("-s"); }
        if (info.gen_data) { compile.emplace_back("-g"); }
        if (info.debug) { compile.emplace_back("-d"); }
        auto Ret = cmd::run_cmd(compile);
        if (Ret.value.value_or(-1) != 0) { return; }
    }
    cmd::Cmd exec{output_file};
    if (cmd::run_cmd(exec).value.value_or(-1) != 0) {
        loge("execute %s failed", info.output.string().c_str());
    }
}

void handle_arg(int argc, char** argv) {
    argp::Parser parser;
    parser.add_opt({"-h", "--help"}, "print helper", argp::Boundary::get_self);
    parser.add_opt({"-v", "--version"}, "get version",
                   argp::Boundary::get_self);

    parser.add_opt(
        {"-n", "--name"},
        "name a special dir,generate exe in dir which belong to rsc's "
        "scripts dir  (scripts/dir/foo.exe)",
        argp::Boundary::get_self);

    parser.add_opt({"-l", "--link"}, "link rsc lib, (default is shared lib)",
                   argp::Boundary::get_self);
    parser.add_opt({"-s", "--static"},
                   "link target with static lib, must have -l arg",
                   argp::Boundary::get_self);
    parser.add_opt({"-g"}, "generate compile_command.json, will override file",
                   argp::Boundary::get_self);
    parser.add_opt({"-o", "--output"}, "output exec file at dir or path",
                   argp::Boundary::one_arg);
    parser.add_opt({"-d"}, "generate with debug info",
                   argp::Boundary::get_self);

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

    auto inputs = parser.get_pos(0);
    if (inputs.empty()) {
        loge("no input file");
        return;
    }
    Info info;
    info.input = {inputs[0]};
    std::string name =
        info.input.filename().replace_extension(get_extension()).string();

    std::string_view dir = parser.get_arg("-n", "");
    info.link = !parser.get_args("-l").empty();
    info.use_static = !parser.get_args("-s").empty();
    info.gen_data = !parser.get_args("-g").empty();
    info.debug = !parser.get_args("-d").empty();
    Path output = parser.get_arg("-o", "");
    if (!output.empty()) {
        if (fs::is_directory(output)) { output = output / name; }
    } else {
        if (dir.empty()) {
            output = rsc::get::script_dir() / name;
        } else {
            if (os::valid_name(dir))
                output = rsc::get::script_dir() / dir / name;
            else {
                loge("%s is invalid dir name", dir.data());
                return;
            }
        }
    }
    handle_compile(info);
}

int main(int argc, char* argv[]) {
    // default install binary as current path
    rsc::init();
    handle_arg(argc, argv);

    return 0;
}
