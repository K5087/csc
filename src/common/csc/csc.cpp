#include <csc/csc.h>
#include <csc/graph.h>
#include <log/log.h>

#include <csc/debug.hpp>

namespace csc {
using namespace cmd;

bool update_bin(Path bin, const std::vector<Path>& files, const cmd::Cmd& cmd) {
    Path old_bin = bin;
    old_bin += ".old";
#ifdef _WIN32
    if (bin.extension() != ".exe") {
        bin.replace_extension(get_extension());
    }
#endif // _WIN32
    bool exist = std::filesystem::exists(bin);
    if (exist) {
        bool need_rebuild = is_outdated(bin, files);
        if (!need_rebuild) {
            return true;
        }
        std::filesystem::rename(bin, old_bin);
    }

    logi("build program start update");
    if (cmd.empty()) {
        loge("have no default compile command");
        return false;
    }

    if (run_cmd(cmd).value.value_or(-1) != 0) {
        loge("compile build script failed");

        if (exist) {
            std::filesystem::rename(old_bin, bin);
        }
        return false;
    }
    logi("update build program success");

#ifdef _WIN32
#else
    std::filesystem::remove(old_binary_path);
#endif
    return true;
}

bool is_outdated(const Path& file, const std::vector<Path>& inputs) {
    using std::filesystem::last_write_time;

    if (!std::filesystem::exists(file)) { return true; }
    auto ouput_time = last_write_time(file);

    for (size_t i = 0; i < inputs.size(); ++i) {
        auto input_time = last_write_time(inputs[i]);

        if (input_time > ouput_time) {
            return true;
        }
    }
    return false;
}

bool build_target(std::shared_ptr<Target> info) {
    // TODO
    if (!std::filesystem::exists(info->build)) {
        std::filesystem::create_directories(info->build);
    }
    // generate build
    for (auto& build : info->deps) {
        if (!build->is_build) {
            if (!build_target(build)) {
                return false;
            }
        }
    }
    // generate obj
    std::vector<std::string> objs = {};
    std::vector<Ret> rets = {};
    for (auto& unit : info->units) {
        Path obj = info->GetBuildPath(unit, info->build).replace_extension(".o");
        if (!std::filesystem::exists(obj.parent_path())) {
            std::filesystem::create_directories(obj.parent_path());
        }
        objs.push_back(obj.generic_string());
        rets.push_back(compile_unit(*info, unit, objs.back()));
        if (rets.back().value.value_or(-1) != 0) {
            loge("compile %s failed,", unit.c_str());
            return false;
        }
    }

    // wiat compile complete
    cmd::wait_procs(rets);

    bool link;
    // link obj
    switch (info->type) {
        case BuildType::exe: link = link_exe(*info, objs); break;
        case BuildType::lib: link = link_lib(*info, objs); break;
        case BuildType::dll: link = link_dll(*info, objs); break;
    }

    if (link) {
        info->is_build = true;
        logi("%s build success", info->name.c_str());
        return true;
    } else {
        loge("build %s failed,", info->name.c_str());
        return false;
    }
}

bool link_exe(Target& info, const std::vector<std::string>& objs) {
    Cmd link{info.tool_chain->GetCompiler()};

    link.append_range(objs);
    std::vector<std::string> targets;
    for (auto& build : info.deps) {
        targets.emplace_back(build->GetTarget());
        link.push_back(targets.back());
    }
    link.emplace_back("-o");
    auto target = info.GetTarget();
    link.push_back(target);
    print_cmd(link);
    if (run_cmd(link).value.value_or(-1) != 0) {
        loge("link %s failed", info.name.c_str());
        return false;
    }
    return true;
}

// TODO: msvc is different
bool link_lib(Target& info, const std::vector<std::string>& objs) {
    Cmd archive{info.tool_chain->GetArchiver(), "rsc"};
    auto target = info.GetTarget();
    archive.push_back(target);
    archive.append_range(objs);

    if (run_cmd(archive).value.value_or(-1) != 0) {
        loge("archive %s failed", info.name.c_str());
        return false;
    }
    return true;
}

bool link_dll(Target& info, const std::vector<std::string>& objs) {
    Cmd link{info.tool_chain->GetCompiler()};

    link.emplace_back("-shared");

    link.append_range(objs);
    std::vector<std::string> targets;
    for (auto& build : info.deps) {
        targets.emplace_back(build->GetTarget());
        link.push_back(targets.back());
    }
    link.emplace_back("-o");
    auto target = info.GetTarget();
    link.push_back(target);
    if (run_cmd(link).value.value_or(-1) != 0) {
        loge("link %s failed", info.name.c_str());
        return false;
    }
    return true;
}

cmd::Ret compile_unit(const Target& target, const Path& unit, const std::string& obj) {
    std::string unit_path = unit.generic_string();
    Cmd compile{target.tool_chain->GetCompiler(), "-c", unit_path, "-o", obj};
    compile.append_range(target.flags);
    Opt opt;
    opt.wait_return = false;
    auto ret = cmd::run_cmd(compile, opt);
    if (ret.value.value_or(-1) != 0) {
        print_cmd(compile);
    }
    // if (cmd::run_cmd(compile, opt).value.value_or(-1) != 0) {
    //     return false;
    // }
    // return true;
    return ret;
}

} // namespace csc
