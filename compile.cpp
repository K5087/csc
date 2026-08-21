#include <csc/csc.hpp>
#include <csc/debug.hpp>
#include <log/log.h>
#include <rsc/os.h>

// default use absolute path

namespace fs = std::filesystem;

// fs::path current = CURRENT_DIR;
fs::path current = "./";
fs::path build = current / "rsc_build";
std::vector<std::string> flags = {"-std=c++26"};
std::shared_ptr<csc::LLVM> llvm = std::make_shared<csc::LLVM>();

fs::path source = current / "src/common";
#ifdef _WIN32
std::string platform = "windows";
#else
std::string platform = "unix";
#endif // _WIN32
fs::path platform_source = current / "src" / platform;

csc::Build make_build(const std::string& name) {
    auto target = csc::make::static_target(name);
    target->build = build;
    target->tool_chain = llvm;
    auto sources = csc::find_file(
        {source / target->name, platform_source / target->name}, ".cpp");
    target->units.append_range(sources);
    target->flags.append_range(flags);
    target->AddIncludes({current / "include"});
    return target;
}

void install(const std::vector<fs::path>& bins, std::vector<fs::path>& libs) {
    fs::path root = os::get_home_dir() / "rsc/packages/rsc/1.0.0";
    fs::create_directories(root);
    fs::path bin_dir = root / "bin";
    fs::create_directories(bin_dir);
    fs::path lib_dir = root / "lib";
    fs::create_directories(lib_dir);

    for (auto& path : bins) {
        fs::copy(path, bin_dir, fs::copy_options::update_existing);
    }
    for (auto& path : libs) {
        fs::copy(path, lib_dir, fs::copy_options::update_existing);
    }
}

void update_self(int argc, char** argv) {
    auto sources = csc::find_file({source, platform_source}, ".cpp");
    sources.push_back(current / "compile.cpp");
    std::vector<fs::path> includes = {current / "include"};
    auto output = fs::relative(argv[0], current);
    auto command = csc::make_compile_cmd(sources, includes, output);
    std::vector<fs::path> check_files = sources;
    check_files.emplace_back(__FILE__);
    cmd::Cmd cmd;
    cmd.append_range(command);
    cmd.append_range(flags);
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

    // build rsc.exe compile.exe
    auto rsc = make_build("rsc");
    rsc->type = csc::BuildType::exe;
    rsc->searches = {"uuid", "ole32"};
    rsc->AddDepends({log, argp, cmd, csc});

    auto compile = std::make_shared<csc::Target>(*rsc);
    compile->name = "compile";

    rsc->units.push_back(current / "src/main.cpp");
    compile->units.push_back(current / "compile.cpp");

    if (!build_target(rsc)) { return -1; }
    if (!build_target(compile)) { return -1; }
    csc->type = csc::BuildType::dll;
    if (!build_target(csc)) { return -1; }

    // install
    std::vector<fs::path> libs;
    std::vector<fs::path> bins;
    bins.push_back(rsc->GetTarget());
    bins.push_back(compile->GetTarget());
    libs.append_range(std::vector<fs::path>{log->GetTarget(), argp->GetTarget(),
                                            csc->GetTarget(),
                                            cmd->GetTarget()});
    install(bins, libs);
    return 0;
}
