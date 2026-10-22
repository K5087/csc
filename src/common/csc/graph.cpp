#include <csc/graph.h>

#include <csc/debug.hpp>
#include <csc/tool.h>

#include <algorithm>

namespace csc {
Graph::Graph(Target& target) : target(target) {
    for (auto& unit : target.units) {
        units.emplace_back(unit, target.GetBuildPath(unit, target.build));
    }
}

bool Graph::parse_p1689(Unit& unit) {
    auto json = read_file(unit.p1689);
    if (!json) { return false; }

    std::size_t pos = 0;
    auto value = json::parse(*json, pos);
    if (!value) { return false; }
    if (!decode(*value, unit.info)) { return false; }
    return true;
}

bool Graph::build() {
    for (auto& unit : units) {
        if (!fs::exists(unit.p1689)) {
            rets.push_back(gen_p1689(unit));
            if (rets.back().value.value_or(-1) != 0) {
                loge("generate p1689 for %s failed", unit.p1689_path.c_str());
                return false;
            }
        }
    }
    cmd::wait_procs(rets);
    for (auto& ret : rets) {
        if (auto code = ret.value.value_or(-1); code != 0) {
            loge("generate p1689 process return: %d", code);
            return false;
        }
    }

    for (auto& unit : units) {
        if (!parse_p1689(unit)) {
            loge("parse p1689 for %s failed", unit.p1689_path.c_str());
            return false;
        }
    }
    if (!recover_depend()) {
        loge("recover depend relation failed");
        return false;
    }

    if (have_cycle()) { return false; }

    rets.clear();
    for (std::size_t i = 0; i < units.size(); i++) {
        tasks.emplace_back(std::make_shared<Task>(i));
    }
    for (auto& task : tasks) { handle_task(task); }

    while (!rets.empty()) {
        auto opt = cmd::wait_any(rets);
        if (!opt) {
            loge("wait task failed");
            return false;
        }
        auto index = *opt;
        rets.erase(rets.begin() + index);
        auto task_index = ret_map[index];
        ret_map.erase(index);
        handle_task(tasks[task_index]);
    }

    std::vector<fs::path> deps;
    for (const auto& dep : target.deps) { deps.push_back(dep->GetTarget()); };
    if (!need_rebuild && !is_outdated(target.GetTarget(), deps)) {
        target.is_build = true;
        logi("%s %s not need to update", target.name.c_str(),
             Serialize(target.type).c_str());
        return true;
    }

    bool link;
    // link obj
    std::vector<std::string> objs{};
    objs.reserve(units.size());
    for (auto& unit : units) { objs.push_back(unit.obj_path); }
    switch (target.type) {
        case TargetType::exec: link = impl::link_exe(target, objs); break;
        case TargetType::arch: link = impl::link_lib(target, objs); break;
        case TargetType::share: link = impl::link_dll(target, objs); break;
    }

    return link;
}

Task::Task(std::size_t index) : index(index) {}

void Graph::handle_task(std::shared_ptr<Task> task) {
    auto& unit = units[task->index];
    switch (task->status) {
        case Status::bmi: {
            // generate bmi
            for (auto& [name, index] : unit.require) {
                auto& bmi = units[index].pcm;
                if (!is_newest(index)) {
                    module_dep_map[index].insert(task);
                    return;
                }
            }
            task->status = Status::object;
            auto deps = impl::get_deps(unit.obj, target.root);
            if (unit.is_module && is_outdated(unit.pcm, deps)) {
                rets.push_back(gen_bmi(unit));
                break;
            }
        }
        case Status::object: {
            if (auto it = module_dep_map.find(task->index);
                it != module_dep_map.end()) {
                auto& subtasks = it->second;

                while (!subtasks.empty()) {
                    auto it = subtasks.begin();

                    auto subtask = *it;
                    subtasks.erase(it);

                    handle_task(subtask);
                }
            }
            auto deps = impl::get_deps(unit.obj, target.root);
            if (is_outdated(unit.obj, deps)) {
                need_rebuild = true;
                rets.push_back(compile_module(unit));
                break;
            }
            task->status = Status::end;
        }
        case Status::end: return;
        default: throw std::runtime_error("unexpected task status");
    }
    // task index same as unit index
    if (rets.back().proc) { ret_map.insert({rets.size() - 1, task->index}); }
}

bool Graph::recover_depend() {
    ref_map<const fs::path, std::vector<std::size_t>> source_req_map;

    ref_map<const std::string, std::vector<std::size_t>> logical_req_map;

    for (std::size_t i = 0; i < units.size(); i++) {
        auto& unit = units[i];
        for (auto& rule : unit.info.rules) {
            for (auto& provide : rule.provides) {
                unit.is_module = true;
                if (provide.unique_on_source_path) {
                    auto& path = *provide.source_path;
                    auto it = source_map.try_emplace(path, i);
                    if (!it.second) {
                        loge("can't have same module path: %s",
                             path.generic_string().c_str());
                        return false;
                    }
                    if (auto ptr = source_req_map.find(path);
                        ptr != source_req_map.end()) {
                        for (auto& index : ptr->second) {
                            units[index].require.emplace_back(
                                provide.logical_name, i);
                        }
                    }
                } else {
                    auto& name = provide.logical_name;
                    auto it = logical_map.try_emplace(name, i);
                    if (!it.second) {
                        loge("can't have same module name: %s", name.c_str());
                        return false;
                    }
                    if (auto ptr = logical_req_map.find(name);
                        ptr != logical_req_map.end()) {
                        for (auto& index : ptr->second) {
                            units[index].require.emplace_back(
                                provide.logical_name, i);
                        }
                    }
                }
            }
            for (auto& require : rule.require_files) {
                if (require.unique_on_source_path) {
                    if (auto it = source_map.find(*require.source_path);
                        it != source_map.end()) {
                        unit.require.emplace_back(require.logical_name,
                                                  it->second);
                    } else {
                        if (auto it = source_req_map.find(*require.source_path);
                            it != source_req_map.end()) {
                            it->second.push_back(i);
                        } else {
                            source_req_map[*require.source_path] = {i};
                        }
                    }

                } else {
                    if (auto it = logical_map.find(require.logical_name);
                        it != logical_map.end()) {
                        unit.require.emplace_back(require.logical_name,
                                                  it->second);
                    } else {
                        logical_req_map[require.logical_name].push_back(i);
                    }
                }
            }
        }
    }
    return true;
}

bool Graph::have_cycle() {
    enum class Color { white, gray, black };
    std::vector<Color> graph{units.size(), Color::white};
    std::vector<std::size_t> stack;
    std::vector<std::size_t> cycle;
    auto dfs = [&](auto&& self, std::size_t index) {
        graph[index] = Color::gray;
        stack.push_back(index);
        for (auto& [_, dep] : units[index].require) {
            if (graph[dep] == Color::gray) {
                auto it = std::ranges::find(stack, dep);
                cycle.assign(it, stack.end());
                cycle.push_back(dep);
                return true;
            } else if (graph[dep] == Color::white) {
                if (self(self, dep)) { return true; }
            }
        }
        stack.pop_back();
        graph[index] = Color::black;
        return false;
    };
    for (std::size_t i = 0; i < units.size(); i++) {
        if (graph[i] == Color::white) {
            if (dfs(dfs, i)) {
                std::string infos;
                bool is_first = true;
                for (auto& index : cycle) {
                    if (is_first) {
                        is_first = false;
                    } else {
                        infos += " ";
                    }
                    infos += units[index]
                                 .info.rules[0]
                                 .require_files[0]
                                 .logical_name;
                }
                loge("checked cycle depend\n %s", infos.c_str());
                return true;
            }
        }
    }
    return false;
}

cmd::Ret Graph::gen_bmi(const Unit& unit) const {
    cmd::Cmd generate{target.tool_chain->GetCompiler(), "--precompile",
                      unit.path_string};
    generate.reserve(generate.size() + unit.require.size() + 2 +
                     target.flags.size());
    std::vector<std::string> cache;
    for (auto& [name, index] : unit.require) {
        cache.push_back("-fmodule-file=" + name + "=" +
                        units[index].pcm.generic_string());
        generate.push_back(cache.back());
    }
    generate.emplace_back("-o");
    generate.push_back(unit.pcm_path);
    generate.emplace_back("-MMD");
    generate.append_range(target.flags);

    if (target.print_cmd) { print_cmd(generate); }
    return cmd::run_cmd(generate, {.wait_return = false});
}

cmd::Ret Graph::gen_p1689(const Unit& unit) {
    cmd::Cmd generate{
        target.tool_chain->GetScaner(),
        "-format=p1689",
        "-o",
        unit.p1689_path,
        "--",
        target.tool_chain->GetCompiler(),
        "-c",
        unit.path_string,
        "-o",
        unit.obj_path,
    };
    // ISSUE:"-MMD" clang-scan will include system header
    generate.append_range(target.flags);
    if (target.print_cmd) { print_cmd(generate); }
    return cmd::run_cmd(generate, {.wait_return = false});
}

cmd::Ret Graph::compile_module(const Unit& unit) const {
    std::vector<std::string> cache;
    cmd::Cmd compile{target.tool_chain->GetCompiler(), "-c"};
    compile.reserve(compile.size() + 1 + unit.require.size() + 3 +
                    target.flags.size());
    if (unit.is_module) {
        compile.emplace_back(unit.pcm_path);
    } else {
        compile.emplace_back(unit.path_string);
    }
    for (auto& [name, index] : unit.require) {
        cache.push_back("-fmodule-file=" + name + "=" +
                        units[index].pcm.generic_string());
        compile.push_back(cache.back());
    }
    compile.emplace_back("-o");
    compile.push_back(unit.obj_path);
    if (!unit.is_module) { compile.emplace_back("-MMD"); }
    compile.append_range(target.flags);

    if (target.print_cmd) { print_cmd(compile); }
    return cmd::run_cmd(compile, {.wait_return = false});
}

bool Graph::is_newest(std::size_t index) const {
    return index < tasks.size() && tasks[index]->status > Status::bmi;
}
} // namespace csc
