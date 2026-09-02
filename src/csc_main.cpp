#include <argp/argp.h>
#include <csc/csc.h>
#include <csc/target.h>
#include <csc/tool.h>
#include <csc/tool_chain.h>
#include <log/log.h>

using namespace csc;
namespace fs = std::filesystem;

std::string serialize(const std::vector<fs::path>& paths) {
    std::string str = "{";
    bool is_first = true;
    for (auto& path : paths) {
        if (is_first)
            is_first = false;
        else
            str += ",";
        str += "\"" + path.generic_string() + "\"";
    }

    str += "}";
    return str;
}

/* have path mean will generate in path, name will not use*/
void handle_compile(const fs::path& csc, const fs::path& input,
                    const fs::path& output, std::vector<std::string_view> links,
                    bool link = false, bool use_static = false,
                    bool gen_data = false) {
    std::string input_file = input.generic_string();
    std::string output_file = output.generic_string();

    std::vector<std::string> args;
    cmd::Cmd compile{get_default_compiler(), input_file};
    if (link) {
        // rsc common bin path
        auto package_dir =
            csc.parent_path().parent_path() / "package/rsc/current";
        // rsc package bin path
        if (!fs::exists(package_dir))
            package_dir = csc.parent_path().parent_path();
        auto shared_dir = package_dir / "lib/shared";
        auto include_dir = package_dir / "include";
        args.emplace_back("-I" + include_dir.generic_string());
        args.emplace_back("-DCSC_INCLUDE_DIR=" + serialize({include_dir}));

        if (use_static) {
            auto static_dir = package_dir / "lib/static";
            std::vector<fs::path> libs{
                static_dir / "log.a",
                static_dir / "argp.a",
                static_dir / "cmd.a",
                static_dir / "csc.a",
            };
            for (auto& lib : libs) { args.emplace_back(lib.generic_string()); }
            libs.append_range(links);
            args.emplace_back("-DCSC_LINK_SHARED_LIB=" + serialize(libs));
        } else {
#ifdef _WIN32
            std::string shared = (shared_dir / "csc.dll").generic_string();
#else
            std::string shared = (shared / "csc.so").string();
#endif // _WIN32
            std::vector<fs::path> libs{shared};
            args.emplace_back(shared);
            libs.append_range(links);
            args.emplace_back("-DCSC_LINK_SHARED_LIB=" + serialize({libs}));
        }
    } else if (use_static) {
        logw("use static muse have -l arg");
    }
    compile.append_range(args);
    compile.append_range(links);

    compile.emplace_back("-o");
    compile.emplace_back(output_file);
    compile.emplace_back("-std=c++26");
    if (gen_data) {
        std::string database = "[\n";
        impl::gen_database(database,
                           fs::absolute(input.parent_path()).generic_string(),
                           compile, input_file, output_file);
        database += "\n]";
        fs::path path = output.parent_path() / "compile_commands.json";
        if (!write_file(path, database)) {
            loge("gen compile commands failed: %s",
                 path.generic_string().c_str());
            goto skip_gen;
        }
        args.push_back("-DCSC_GEN_DATABASE=" + path.generic_string());
        compile.emplace_back(args.back());
    }

skip_gen:

    if (csc::update_bin(output, {input}, compile) ==
        csc::UpdateStatus::failed) {
        loge("build %s failed", output.filename().string().c_str());
        return;
    }

    logi("build %s success", output.generic_string().c_str());
}

void handle_arg(int argc, char** argv) {
    argp::Parser parser;
    parser.add_opt({"-h", "--help"}, "print helper", argp::Boundary::get_self);
    parser.add_opt({"-v", "--version"}, "get version",
                   argp::Boundary::get_self);

    parser.add_opt({"-n", "--name"}, "named build,expect explicit name",
                   argp::Boundary::get_self);
    parser.add_opt({"-l", "--link"}, "link csc lib, (default is shared lib)",
                   argp::Boundary::get_self);
    parser.add_opt({"-s", "--static"},
                   "link target with static lib,must have -l arg",
                   argp::Boundary::get_self);
    parser.add_opt({"-g"}, "generate compile_command.json, will override file",
                   argp::Boundary::get_self);
    parser.add_opt({"-o", "--output"}, "output exec file at dir or path",
                   argp::Boundary::one_arg);
    parser.add_opt({"-L"}, "link others lib(use path)",
                   argp::Boundary::another_rule);

    parser.add_pos("input", false, "input script file path",
                   argp::Boundary::another_rule);

    parser.parse(argc, argv);

    /* help */
    if (!parser.get_args("-h").empty()) {
        parser.print_helper("csc");
        return;
    }

    /* version */
    if (!parser.get_args("-v").empty()) {
        printf("csc 1.0.0 build by moke");
        return;
    }

    /* compile args */
    auto inputs = parser.get_pos(0);
    if (inputs.empty()) {
        loge("no input file");
        return;
    }
    fs::path input = {inputs[0]};
    std::string default_name =
        input.filename().replace_extension(get_extension()).string();
    std::string_view name = parser.get_arg("-n", default_name);
    bool use_link = !parser.get_args("-l").empty();
    bool use_static = !parser.get_args("-s").empty();
    bool gen_data = !parser.get_args("-g").empty();
    Path output = parser.get_arg("-o", "");

    if (output.empty()) {
        output = fs::current_path() / name;
    } else if (fs::exists(output)) {
        if (fs::is_directory(output)) { output = output / name; }
    } else {
        if (output.filename().empty()) {
            fs::create_directory(output);
            output = output / name;
        } else {
            fs::create_directory(output.parent_path());
        }
    }

    std::vector<std::string_view> links = parser.get_args("-L");

    handle_compile(argv[0], input, output, links, use_link, use_static,
                   gen_data);
}

int main(int argc, char* argv[]) {
    // default install binary as current path
    handle_arg(argc, argv);

    return 0;
}
