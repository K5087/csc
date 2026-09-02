#include <argp/argp.h>
#include <csc/target.h>
#include <csc/tool_chain.h>
#include <log/log.h>
#include <rsc/os.h>
#include <rsc/rsc.h>

using namespace csc;
namespace fs = std::filesystem;

/* have path mean will generate in path, name will not use*/
void handle_compile(const fs::path& input, const fs::path& output,
                    bool link = false, bool use_static = false,
                    bool gen_data = false) {
    std::string input_file = input.generic_string();
    std::string output_file = output.generic_string();
    if (!fs::exists(output)) {
        fs::path package_dir = rsc::get::package_dir("rsc");
        std::string csc =
            (package_dir / ("bin/csc" + std::string(get_extension()))).string();
        std::string rsc;
        if (use_static) {
            rsc = (package_dir / ("lib/static/rsc" +
                                  std::string(get_extension(TargetType::arch))))
                      .string();

        } else {
            rsc =
                (package_dir / ("lib/shared/rsc" +
                                std::string(get_extension(TargetType::share))))
                    .string();
        }
        cmd::Cmd compile{csc, input_file, "-o", output_file, "-L", rsc};
        if (link) { compile.emplace_back("-l"); }
        if (use_static) { compile.emplace_back("-s"); }
        if (gen_data) { compile.emplace_back("-g"); }
        auto Ret = cmd::run_cmd(compile);
        if (Ret.value.value_or(-1) != 0) { return; }
    }
    cmd::Cmd exec{output_file};
    if (cmd::run_cmd(exec).value.value_or(-1) != 0) {
        loge("execute %s failed", output.string().c_str());
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
    fs::path input = {inputs[0]};
    std::string name =
        input.filename().replace_extension(get_extension()).string();

    std::string_view dir = parser.get_arg("-n", "");
    bool use_link = !parser.get_args("-l").empty();
    bool use_static = !parser.get_args("-s").empty();
    bool gen_data = !parser.get_args("-g").empty();
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
    handle_compile(input, output, use_link, use_static, gen_data);
}

int main(int argc, char* argv[]) {
    // default install binary as current path
    rsc::init();
    handle_arg(argc, argv);

    return 0;
}
