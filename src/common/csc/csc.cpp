#include <csc/csc.h>
#include <csc/target.h>
#include <csc/tool.h>
#include <log/log.h>

#include <csc/debug.hpp>

#include <cassert>
#include <map>
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
    std::filesystem::remove(old_bin);
#endif
    return UpdateStatus::success;
}

// TODO: for a lot of file ,file access maybe slow
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
                  std::function<void()> before_link) {
    std::filesystem::create_directories(target->build);

    // check deps has build
    for (auto& build : target->deps) {
        if (!build->is_build) {
            // generate deps
            if (!build_target(build)) { return false; }
        }
    }

    std::vector<fs::path> deps;

    // generate obj
    bool need_rebuild = false;
    std::vector<Ret> rets = {};
    std::vector<std::string> objs = {};
    for (auto& unit : target->units) {
        fs::path obj =
            target->GetBuildPath(unit, target->build).replace_extension(".o");
        deps.push_back(obj);
        std::filesystem::create_directories(obj.parent_path());
        objs.push_back(obj.generic_string());
        if (!is_outdated(obj, impl::get_deps(obj, target->root))) { continue; }
        need_rebuild = true;
        rets.push_back(impl::compile_unit(*target, unit, objs.back()));
        if (rets.back().value.value_or(-1) != 0) {
            loge("compile %s failed,", unit.c_str());
            return false;
        }
    }

    for (const auto& dep : target->deps) { deps.push_back(dep->GetTarget()); };
    if (!need_rebuild && !is_outdated(target->GetTarget(), deps)) {
        target->is_build = true;
        logi("%s %s not need to update", target->name.c_str(),
             Serialize(target->type).c_str());
        return true;
    }

    if (before_link) before_link();
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
    return cmd::run_cmd(compile, opt);
}

std::vector<fs::path> get_deps(const fs::path& obj, const fs::path& root) {
    assert(obj.extension() == ".o");
    fs::path dep_file = Path(obj).replace_extension(".d");
    auto ret = read_file(dep_file);
    if (!ret) return {};
    std::vector<fs::path> deps = parse_dep(*ret, root);
    return deps;
}

void gen_database(std::string& o, const std::string& directory,
                  const cmd::Cmd& cmd, const std::string& file,
                  const std::string& output) {
    // unit
    o += "{\n";

    // directory
    o += "  \"directory\": \"" + directory + "\",\n";
    // arguments
    o += "  \"arguments\": [";
    bool is_first = true;
    for (auto& arg : cmd) {
        if (is_first) {
            is_first = false;
        } else {
            o += ", ";
        }
        if (cmd::need_escape(arg)) {
            o += cmd::escape_string(arg);
        } else {
            o += "\"" + std::string(arg) + "\"";
        }
    }
    o += "],\n";

    // o
    o += "  \"file\": \"" + file + "\"";
    // output
    if (!output.empty()) {
        o += ",\n";
        o += "  \"output\": \"" + output + "\"\n";
    }
    o += "}";
}

void gen_database(std::string& database, std::shared_ptr<Target> target,
                  bool append) {
    std::map<std::shared_ptr<Target>, bool> map;
    bool first = true;
    auto pos = database.find_last_of("]");
    if (pos != std::string::npos) {
        database.erase(pos);
        if (auto pos2 = database.find_last_of("}"); pos2 != std::string::npos) {
            database.erase(pos2 + 1);
            first = false;
        }
    }

    auto serialize = [&](auto&& self, std::shared_ptr<Target> target) -> void {
        if (map[target]) return;
        for (auto& dep : target->deps) { self(self, dep); }
        for (auto& unit : target->units) {
            if (first) {
                first = false;
            } else {
                database += ",\n";
            }
            std::string unit_path = unit.generic_string();
            std::string obj =
                Path(unit).replace_extension(".o").generic_string();
            Cmd compile{target->tool_chain->GetCompiler(), "-c", unit_path,
                        "-o", obj};
            compile.append_range(target->flags);

            impl::gen_database(database, target->root.generic_string(), compile,
                               unit.filename().string(), obj);
        }
        map[target] = true;
    };
    serialize(serialize, target);
    database += "\n]";
}
} // namespace impl

void gen_database(std::shared_ptr<Target> target, const fs::path& path,
                  bool append) {
    std::map<std::shared_ptr<Target>, bool> map;
    std::string database;
    bool first = true;
    if (append) {
        database = read_file(path).value_or("[\n");
    } else {
        database = "[\n";
    }

    impl::gen_database(database, target, append);

    if (!write_file(path, database)) {
        loge("write file failed: %s", path.string().c_str());
        return;
    }
}

} // namespace csc
