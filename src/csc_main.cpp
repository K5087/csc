#include <argp/argp.h>
#include <csc/debug.hpp>
#include <csc/target.h>
#include <csc/tool_chain.h>
#include <log/log.h>

using namespace csc;
namespace fs = std::filesystem;
fs::path current = fs::current_path();

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
void handle_run(std::string_view input, std::string_view name,
                std::vector<std::string_view> links, const Path& path = {},
                bool link = false, bool use_static = false) {
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
        dir = current / source.stem();
    } else {
        dir = current / name;
    }

    if (output.empty()) {
        output = (dir / source.filename())
                     .replace_extension(get_extension())
                     .generic_string();
    }

    std::filesystem::create_directory(dir);

    std::vector<std::string> args;
    cmd::Cmd compile{get_default_compiler(), input};
    if (link) {
        auto package_dir = current.parent_path();
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
            // TODO: deside links other lib should serialize?
            args.emplace_back("-DCSC_LINK_SHARED_LIB=" + serialize(libs));
        } else {
#ifdef _WIN32
            std::string shared = (shared_dir / "csc.dll").generic_string();
#else
            std::string shared = (shared / "csc.so").string();
#endif // _WIN32
            args.emplace_back(shared);
            args.emplace_back("-DCSC_LINK_SHARED_LIB=" + serialize({shared}));
        }
    } else if (use_static) {
        logw("use static muse have -l arg");
    }
    compile.append_range(args);
    compile.append_range(links);

    compile.emplace_back("-o");
    compile.emplace_back(output);
    compile.emplace_back("-std=c++26");

    if (csc::update_bin(input, {source}, compile) ==
        csc::UpdateStatus::failed) {
        loge("build %s failed", source.filename().string().c_str());
        return;
    }

    logi("build %s success", source.filename().string().c_str());
    cmd::Cmd exec{output};
    if (cmd::run_cmd(exec).value.value_or(-1) != 0) {
        loge("execute %s failed", output.c_str());
    }
}

void handle_arg(int argc, char** argv) {
    argp::Parser parser;
    parser.add_opt({"-h", "--help"}, "print helper", argp::Boundary::get_self);
    parser.add_opt({"-v", "--version"}, "get version",
                   argp::Boundary::get_self);

    parser.add_opt({"-n", "--name"}, "named build,excape explicit name",
                   argp::Boundary::get_self);
    parser.add_opt({"-l", "--link"}, "link csc lib, (default is shared lib)",
                   argp::Boundary::get_self);
    parser.add_opt({"-s", "--static"},
                   "link target with static lib,must have -l arg",
                   argp::Boundary::get_self);
    parser.add_opt({"-o", "--output"}, "output exec file at dir or path",
                   argp::Boundary::one_arg);
    parser.add_opt({"-L"}, "link others shared lib",
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

    std::string_view name = parser.get_arg("-n", "");
    bool use_link = !parser.get_args("-l").empty();
    bool use_static = !parser.get_args("-s").empty();
    Path output = parser.get_arg("-o", "");
    std::vector<std::string_view> links = parser.get_args("-L");

    if (links.empty()) {
        loge("-L must have args");
        return;
    }

    auto inputs = parser.get_pos(0);
    if (inputs.empty()) {
        loge("no input file");
        return;
    }
    handle_run(inputs[0], name, links, output, use_link, use_static);
}

int main(int argc, char* argv[]) {
    // default install binary as current path
    handle_arg(argc, argv);

    return 0;
}
