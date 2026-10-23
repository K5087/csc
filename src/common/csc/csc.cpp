#include <csc/csc.h>
#include <csc/depend.h>
#include <csc/graph.h>
#include <csc/target.h>
#include <csc/tool.h>
#include <log/log.h>

#include <csc/debug.hpp>

#include <cassert>
#include <map>
#include <ranges>
#include <set>

namespace csc {
using namespace cmd;

bool update_bin(fs::path bin, const cmd::Cmd& cmd) {
#ifdef _WIN32
    if (bin.extension() != ".exe") { bin.replace_extension(get_extension()); }
#endif // _WIN32
    fs::path old_bin = bin;
    old_bin += ".old";
    bool exist = std::filesystem::exists(bin);
    if (exist) {
        std::filesystem::rename(bin, old_bin);
    } else {
        loge("%s not exist", bin.generic_string().c_str());
        return false;
    }

    logi("update build program start");
    if (cmd.empty()) {
        loge("have no default compile command");
        return false;
    }

    if (run_cmd(cmd).value.value_or(-1) != 0) {
        loge("compile build script failed");

        if (exist) { std::filesystem::rename(old_bin, bin); }
        return false;
    }
    logi("update build program success");

#ifdef _WIN32
#else
    std::filesystem::remove(old_bin);
#endif
    return true;
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
        target->includes.insert_range(build->includes);
        if (!build->is_build) {
            // generate deps
            if (!build_target(build)) { return false; }
        }
    }

    for (auto& include : target->includes) {
        target->flags.push_back("-I" + include.generic_string());
    }

    std::vector<fs::path> deps;

    // generate obj and depend file
    bool need_rebuild = false;
    std::vector<Ret> rets = {};
    std::vector<std::string> objs = {};
    for (auto& unit : target->units) {
        fs::path obj =
            target->GetBuildPath(unit, target->build).replace_extension(".o");
        deps.push_back(obj);
        objs.push_back(obj.generic_string());
        if (!is_outdated(obj, impl::get_deps(obj, target->root))) { continue; }
        std::filesystem::create_directories(obj.parent_path());
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
        case TargetType::head: return true; // this won't happend
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
        loge("%s %sbuild failed", target->name.c_str(),
             Serialize(target->type).c_str());
        return false;
    }
}

bool build_graph(std::shared_ptr<Target> target,
                 std::function<void()> before_link) {
    std::filesystem::create_directories(target->build);
    // check deps has build
    for (auto& build : target->deps) {
        target->includes.insert_range(build->includes);
        if (!build->is_build) {
            // generate deps
            if (!build_graph(build)) { return false; }
        }
    }
    for (auto& include : target->includes) {
        target->flags.push_back("-I" + include.generic_string());
    }
    Graph graph(*target);
    bool ret = graph.build();
    if (ret) {
        target->is_build = true;
        logi("%s %s build success", target->name.c_str(),
             Serialize(target->type).c_str());
        return true;
    } else {
        loge("%s %s build failed", target->name.c_str(),
             Serialize(target->type).c_str());
        return false;
    }
    return ret;
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
void Append_Deps(std::set<std::string>& deps, const csc::Target& target) {
    for (auto& dep : target.deps) {
        if (dep->type == TargetType::arch) {
            deps.insert(dep->GetTarget());
            Append_Deps(deps, *dep);
        }
    }
}

bool link_exe(Target& info, const std::vector<std::string>& objs) {
    Cmd link{info.tool_chain->GetCompiler()};
    link.append_range(objs);
    std::set<std::string> deps;
    Append_Deps(deps, info);
    for (auto& lib : info.searches) { deps.emplace("-l" + lib); }
    link.append_range(deps);
    link.emplace_back("-o");
    auto target = info.GetTarget();
    link.push_back(target);

    if (info.print_cmd) { print_cmd(link); }
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

    if (info.print_cmd) { print_cmd(archive); }
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
    std::set<std::string> deps;
    Append_Deps(deps, info);
    for (auto& lib : info.searches) { deps.emplace("-l" + lib); }
    link.append_range(deps);
    link.emplace_back("-o");
    auto target = info.GetTarget();
    link.push_back(target);

    if (info.print_cmd) { print_cmd(link); }
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
    if (target.print_cmd) { print_cmd(compile); }
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
            std::string unit_path =
                fs::relative(unit, target->root).generic_string();
            std::string obj =
                fs::relative(target->GetBuildPath(unit, target->build)
                                 .replace_extension(".o"),
                             target->root)
                    .generic_string();
            Cmd compile{target->tool_chain->GetCompiler(), "-c", unit_path,
                        "-o", obj};
            compile.append_range(target->flags);

            impl::gen_database(database, target->root.generic_string(), compile,
                               unit_path, obj);
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
