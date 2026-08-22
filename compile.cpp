#include <csc/csc.hpp>
#include <csc/debug.hpp>
#include <log/log.h>
#include <rsc/rsc.h>

namespace fs = std::filesystem;

/* project setting */
const char* version = "1.0.0";

/* predefine path */
fs::path current = CURRENT_DIR;
// fs::path current = "./";
fs::path build_dir = current / "rsc_build";
fs::path include_dir = current / "include";
std::vector<std::string> flags = {"-std=c++26"};
std::shared_ptr<csc::LLVM> llvm = std::make_shared<csc::LLVM>();

/* project path */
fs::path source = current / "src/common";
#ifdef _WIN32
std::string platform = "windows";
#else
std::string platform = "unix";
#endif // _WIN32
fs::path platform_source = current / "src" / platform;

csc::Build make_build(const std::string& name) {
    auto target = csc::make::static_target(name);
    target->build = build_dir;
    target->tool_chain = llvm;
    auto sources = csc::find_file(
        {source / target->name, platform_source / target->name}, ".cpp");
    target->units.append_range(sources);
    target->flags.append_range(flags);
    target->AddIncludes({current / "include"});
    return target;
}

csc::Build make_shared(csc::Build target) {
    csc::Build shared = std::make_shared<csc::Target>(*target);
    shared->type = csc::TargetType::share;
    return shared;
}

void update_self(int argc, char** argv) {
    auto sources = csc::find_file({source, platform_source}, ".cpp");
    sources.push_back(current / "compile.cpp");
    std::vector<fs::path> includes = {include_dir};
    auto output = fs::relative(argv[0], current);
    auto command = csc::make_compile_cmd(sources, includes, output);
    std::vector<fs::path> check_files = sources;
    check_files.emplace_back(__FILE__);
    cmd::Cmd cmd;
    cmd.append_range(command);
    cmd.emplace_back("-luuid");
    cmd.emplace_back("-lole32");

    print_cmd(cmd);
    csc::update_self(argc, argv, check_files, cmd);
}

int main(int argc, char* argv[]) {
    // update if need
    update_self(argc, argv);

    // build lib
    auto log = make_build("log");
    auto argp = make_build("argp");

    auto cmd = make_build("cmd");
    cmd->AddDepend(log);

    auto csc = make_build("csc");
    csc->AddDepends({log, cmd});
    auto package_dir = rsc::get::package_dir("rsc", version);
    csc->flags.emplace_back("-DCSC_INSTALL_DIR=\"" +
                            package_dir.generic_string() + "\"");

    auto csc_shared = make_shared(csc);

    // build rsc.exe compile.exe
    auto rsc = make_build("rsc");
    rsc->type = csc::TargetType::exec;
    rsc->searches = {"uuid", "ole32"};
    rsc->AddDepends({log, argp, cmd, csc});

    auto compile = std::make_shared<csc::Target>(*rsc);
    compile->name = "compile";

    auto rsc_shared = make_shared(rsc);

    rsc->units.push_back(current / "src/main.cpp");
    compile->units.push_back(current / "compile.cpp");

    if (!build_target(rsc)) { return -1; }
    if (!build_target(compile)) { return -1; }
    if (!build_target(csc_shared)) { return -1; }
    if (!build_target(rsc_shared)) { return -1; }

    // install
    std::vector<fs::path> bins;
    std::vector<fs::path> includes;
    std::vector<fs::path> static_libs;
    std::vector<fs::path> shared_libs;
    bins.emplace_back(rsc->GetTarget());
    bins.emplace_back(compile->GetTarget());
    includes.emplace_back(include_dir);
    static_libs.append_range(
        std::vector<fs::path>{log->GetTarget(), argp->GetTarget(),
                              csc->GetTarget(), cmd->GetTarget()});
    shared_libs.emplace_back(csc_shared->GetTarget());
    shared_libs.emplace_back(rsc_shared->GetTarget());

    rsc::install("rsc", version, includes, bins, static_libs, shared_libs);
    rsc::set_current("rsc", version);
    rsc::create_shim("rsc", {rsc->GetTarget()}, shared_libs, version);
    return 0;
}
