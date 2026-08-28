#include <csc/csc.h>
#include <csc/target.h>
#include <csc/tool.h>
#include <log/log.h>

#include <csc/debug.hpp>

#include <cassert>
#include <ranges>

namespace csc {
using namespace cmd;

UpdateStatus update_bin(fs::path bin, const std::vector<Path>& files,
                        const cmd::Cmd& cmd) {
    fs::path old_bin = bin;
    old_bin += ".old";
#ifdef _WIN32
    if (bin.extension() != ".exe") { bin.replace_extension(get_extension()); }
#endif // _WIN32
    bool exist = std::filesystem::exists(bin);
    if (exist) {
        bool need_rebuild = is_outdated(bin, files);
        if (!need_rebuild) { return UpdateStatus::noneed; }
        std::filesystem::rename(bin, old_bin);
    }

    logi("update build program start");
    if (cmd.empty()) {
        loge("have no default compile command");
        return UpdateStatus::failed;
    }

    if (run_cmd(cmd).value.value_or(-1) != 0) {
        loge("compile build script failed");

        if (exist) { std::filesystem::rename(old_bin, bin); }
        return UpdateStatus::failed;
    }
    logi("update build program success");

#ifdef _WIN32
#else
    std::filesystem::remove(old_binary_path);
#endif
    return UpdateStatus::success;
}

bool is_outdated(const fs::path& file, const std::vector<Path>& inputs) {
    using std::filesystem::last_write_time;

    if (!std::filesystem::exists(file)) { return true; }
    auto file_name = last_write_time(file);

    for (size_t i = 0; i < inputs.size(); ++i) {
        auto input_time = last_write_time(inputs[i]);

        if (input_time > file_name) { return true; }
    }
    return false;
}

bool build_target(std::shared_ptr<Target> target,
                  std::function<void()> before_build) {
    std::filesystem::create_directories(target->build);

    // check deps has build
    for (auto& build : target->deps) {
        if (!build->is_build) {
            // generate deps
            if (!build_target(build)) { return false; }
        }
    }
    std::vector<fs::path> deps = target->units;

    // check deps file is out of date
    // TODO: .h affect
    // use clang++ -MMD -MF foo.d -c foo.cpp -o foo.o
    for (const auto& dep : target->deps) { deps.push_back(dep->GetTarget()); }
    if (!is_outdated(target->GetTarget(), deps)) {
        target->is_build = true;
        logi("%s %s not need to update", target->name.c_str(),
             Serialize(target->type).c_str());
        return true;
    }

    if (before_build) before_build();

    // generate obj
    bool need_rebuild = false;
    std::vector<Ret> rets = {};
    std::vector<std::string> objs = {};
    for (auto& unit : target->units) {
        fs::path obj =
            target->GetBuildPath(unit, target->build).replace_extension(".o");
        std::filesystem::create_directories(obj.parent_path());
        objs.push_back(obj.generic_string());
        if (!is_outdated(obj, {unit})) {
            need_rebuild = true;
            continue;
        }
        rets.push_back(impl::compile_unit(*target, unit, objs.back()));
        if (rets.back().value.value_or(-1) != 0) {
            loge("compile %s failed,", unit.c_str());
            return false;
        }
    }

    // wiat compile complete
    cmd::wait_procs(rets);
    for (auto& ret : rets) {
        if (ret.value.value_or(-1) != 0) {
            loge("build %s failed,", target->name.c_str());
            return false;
        }
    }

    bool link;
    // link obj
    switch (target->type) {
        case TargetType::exec: link = impl::link_exe(*target, objs); break;
        case TargetType::arch: link = impl::link_lib(*target, objs); break;
        case TargetType::share: link = impl::link_dll(*target, objs); break;
    }

    if (link) {
        target->is_build = true;
        logi("%s %s build success", target->name.c_str(),
             Serialize(target->type).c_str());
        return true;
    } else {
        loge("%s build failed,", target->name.c_str(),
             Serialize(target->type).c_str());
        return false;
    }
}

std::vector<std::string> make_compile_cmd(const std::vector<fs::path>& inputs,
                                          const std::vector<fs::path>& includes,
                                          const fs::path& output) {
    std::vector<std::string> cmd{};
    cmd.emplace_back(get_default_compiler());
    cmd.append_range(inputs | std::views::transform([](const auto& path) {
                         return path.generic_string();
                     }));
    cmd.append_range(includes | std::views::transform([](const auto& path) {
                         return "-I" + path.generic_string();
                     }));
    cmd.emplace_back("-o");
    cmd.emplace_back(output.generic_string());
    cmd.emplace_back("-std=c++26");
    return cmd;
}

namespace impl {

bool link_exe(Target& info, const std::vector<std::string>& objs) {
    Cmd link{info.tool_chain->GetCompiler()};

    link.append_range(objs);
    std::vector<std::string> targets;
    for (auto& build : info.deps) { targets.emplace_back(build->GetTarget()); }
    for (auto& lib : info.searches) { targets.emplace_back("-l" + lib); }
    link.append_range(targets);
    link.emplace_back("-o");
    auto target = info.GetTarget();
    link.push_back(target);
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
    for (auto& build : info.deps) { targets.emplace_back(build->GetTarget()); }
    for (auto& lib : info.searches) { targets.emplace_back("-l" + lib); }
    link.append_range(targets);
    link.emplace_back("-o");
    auto target = info.GetTarget();
    link.push_back(target);
    if (run_cmd(link).value.value_or(-1) != 0) {
        loge("link %s failed", info.name.c_str());
        return false;
    }
    return true;
}

cmd::Ret compile_unit(const Target& target, const fs::path& unit,
                      const std::string& obj) {
    std::string unit_path = unit.generic_string();
    // std::string dep_path =
    // Path(obj).replace_extension(".d").generic_string(); Cmd
    // compile{target.tool_chain->GetCompiler(), "-c", unit_path, "-o", obj,
    // "-MMD","-MF", dep_path};
    Cmd compile{
        target.tool_chain->GetCompiler(), "-c", unit_path, "-o", obj, "-MMD"};
    compile.append_range(target.flags);
    Opt opt;
    opt.wait_return = false;
    auto ret = cmd::run_cmd(compile, opt);
    // TODO: this is a debug
    if (ret.value.value_or(-1) != 0) { print_cmd(compile); }
    return ret;
}

std::vector<fs::path> get_deps(const fs::path& obj) {
    assert(obj.extension() == ".o");
    fs::path dep_file = Path(obj).replace_extension(".d");
    auto ret = read_file(dep_file);
    std::string data = *ret;
    std::vector<fs::path> deps;
}
} // namespace impl

} // namespace csc
