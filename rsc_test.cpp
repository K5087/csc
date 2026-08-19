#include <csc/target.h>
#include <rsc/rsc.h>

// default use absolute path

using namespace csc;
using std::filesystem::absolute;
Path current = Path(__FILE__).parent_path();
std::string platform = "windows";
std::vector<std::string> flags = {"-std=c++26"};
Path source = absolute(current / "src/common");
Path platform_source = absolute(current / "src" / platform);
std::shared_ptr<LLVM> llvm = std::make_shared<LLVM>();
Path build = current / "rsc_build";

std::shared_ptr<Target> build_log() {
    std::shared_ptr<Target> log = std::make_shared<Target>();
    log->root = current;
    log->build = build;
    log->name = "log";
    log->type = BuildType::lib;
    log->tool_chain = llvm;
    auto sources = rsc::find_file({source / log->name, platform_source / log->name}, ".cpp");
    log->units.append_range(sources);
    log->flags.append_range(flags);
    log->AddIncludes({current / "include"});
    return log;
}

std::shared_ptr<Target> build_argp() {
    std::shared_ptr<Target> argp = std::make_shared<Target>();

    argp->root = current;
    argp->build = build;
    argp->name = "argp";
    argp->type = BuildType::lib;
    argp->tool_chain = llvm;
    auto sources = rsc::find_file({source / argp->name, platform_source / argp->name}, ".cpp");
    argp->units.append_range(sources);
    argp->flags.append_range(flags);
    argp->AddIncludes({current / "include"});
    return argp;
}

std::shared_ptr<Target> build_csc() {
    std::shared_ptr<Target> csc = std::make_shared<Target>();
    csc->root = current;
    csc->build = build;
    csc->name = "csc";
    csc->type = BuildType::lib;
    csc->tool_chain = llvm;
    auto sources = rsc::find_file({source / csc->name, platform_source / csc->name}, ".cpp");
    csc->units.append_range(sources);
    csc->flags.append_range(flags);
    csc->AddIncludes({current / "include"});
    return csc;
}

std::shared_ptr<Target> build_cmd() {
    std::shared_ptr<Target> cmd = std::make_shared<Target>();

    cmd->root = current;
    cmd->build = build;
    cmd->name = "cmd";
    cmd->type = BuildType::lib;
    cmd->tool_chain = llvm;
    auto sources = rsc::find_file({source / cmd->name, platform_source / cmd->name}, ".cpp");
    cmd->units.append_range(sources);
    cmd->flags.append_range(flags);
    cmd->AddIncludes({current / "include"});
    return cmd;
}

std::shared_ptr<Target> build_rsc() {
    std::shared_ptr<Target> rsc = std::make_shared<Target>();
    rsc->root = current;
    rsc->build = build;
    rsc->name = "rsc";
    rsc->type = BuildType::exe;
    rsc->tool_chain = llvm;
    auto sources = rsc::find_file({source / rsc->name, platform_source / rsc->name}, ".cpp");
    sources.push_back(current / "src/main.cpp");
    rsc->units.append_range(sources);
    rsc->flags.append_range(flags);
    rsc->AddIncludes({current / "include"});
    return rsc;
}

int main(int argc, char* argv[]) {
    auto log = build_log();
    auto argp = build_argp();
    auto csc = build_csc();
    auto cmd = build_cmd();
    cmd->AddDepend(log);
    csc->AddDepends({log, cmd});

    auto rsc = build_rsc();
    rsc->AddDepends({log, argp, cmd, csc});
    build_target(rsc);
    return 0;
}
