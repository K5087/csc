#include <csc/csc.hpp>
#include <csc/debug.hpp>
#include <log/log.h>
#include <rsc/rsc.h>

using namespace csc;
namespace fs = std::filesystem;

/* project setting */
const char* version = "1.0.0";

/* predefine path */
fs::path current = CURRENT_DIR;
// fs::path current = "./";
fs::path build_dir = current / "rsc_build";
fs::path include_dir = current / "include";
std::vector<std::string> flags = {"-std=c++26"};
std::shared_ptr<LLVM> llvm = std::make_shared<LLVM>();

/* project path */
fs::path source = current / "src/common";
#ifdef _WIN32
std::string platform = "windows";
#else
std::string platform = "unix";
#endif // _WIN32
fs::path platform_source = current / "src" / platform;

Build make_build(const std::string& name) {
    auto target = make::static_target(name);
    target->build = build_dir;
    target->tool_chain = llvm;
    auto sources = find_file(
        {source / target->name, platform_source / target->name}, ".cpp");
    target->units.append_range(sources);
    target->flags.append_range(flags);
    target->AddIncludes({current / "include"});
    return target;
}

Build make_shared(std::shared_ptr<Target> target) {
    auto shared = std::make_shared<Target>(*target);
    shared->type = TargetType::share;
    return shared;
}

bool update_build(int argc, char** argv, std::shared_ptr<Target> build) {
    using std::filesystem::last_write_time;

    fs::path bin = build->GetTarget();
    fs::path old_bin = build->GetTarget() + ".old";
    auto old_time = last_write_time(bin);

    bool exist = fs::exists(bin);

    if (!build_target(build, [=]() {
            if (exist) fs::rename(bin, old_bin);
        })) {
        if (exist) { fs::rename(old_bin, bin); }
        loge("compile build script failed");
        return false;
    }

    auto new_time = last_write_time(bin);
    if (new_time > old_time) {
        logi("*** execute new program ***");
        cmd::Cmd exec;
        for (int i = 0; i < argc; i++) { exec.push_back(argv[i]); }
        cmd::run_cmd(exec);

        // no matter exit is what,when exec_new, old alwase exit
        std::exit(0);
    } else {
        return true;
    }
}

int main(int argc, char* argv[]) {
    // build lib
    auto log = make_build("log");
    auto argp = make_build("argp");

    auto cmd = make_build("cmd");
    cmd->AddDepend(log);

    auto csc = make_build("csc");
    csc->AddDepends({log, cmd, argp});

    auto csc_shared = make_shared(csc);
    auto csc_exe = make_shared(csc);
    csc_exe->type = TargetType::exec;

    // build rsc.exe compile.exe
    auto rsc = make_build("rsc");
    rsc->type = TargetType::exec;
    rsc->searches = {"uuid", "ole32"};
    rsc->AddDepends({log, argp, cmd, csc});

    auto build = std::make_shared<Target>(*rsc);
    build->name = "build";

    auto rsc_shared = make_shared(rsc);

    rsc->units.push_back(current / "src/rsc_main.cpp");
    csc_exe->units.push_back(current / "src/csc_main.cpp");
    build->units.push_back(current / "build.cpp");

    if (!update_build(argc, argv, build)) { return -1; };
    if (!build_target(rsc)) { return -1; }
    if (!build_target(csc_exe)) { return -1; }
    if (!build_target(csc_shared)) { return -1; }
    if (!build_target(rsc_shared)) { return -1; }
    {
        // install
        std::vector<fs::path> bins;
        std::vector<fs::path> includes;
        std::vector<fs::path> static_libs;
        std::vector<fs::path> shared_libs;
        bins.emplace_back(rsc->GetTarget());
        bins.emplace_back(build->GetTarget());
        bins.emplace_back(csc_exe->GetTarget());
        includes.emplace_back(include_dir);
        static_libs.append_range(
            std::vector<fs::path>{log->GetTarget(), argp->GetTarget(),
                                  csc->GetTarget(), cmd->GetTarget()});
        shared_libs.emplace_back(csc_shared->GetTarget());
        shared_libs.emplace_back(rsc_shared->GetTarget());

        rsc::init();
        rsc::install("rsc", version, includes, bins, static_libs, shared_libs);
        rsc::set_current("rsc", version);
        rsc::create_shim("rsc", bins, shared_libs, version);
    }
    return 0;
}
