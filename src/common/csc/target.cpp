#include <csc/target.h>

namespace csc {

std::string Serialize(TargetType type, TargetFlavor flavor) {
    switch (type) {
        case TargetType::exec: return ".exe";
        case TargetType::arch:
            switch (flavor) {
                case TargetFlavor::GNU: return ".a";
                case TargetFlavor::MSVC: return ".lib";
            };
        case TargetType::share: return ".dll";
    }
}

std::string Target::GetTarget() const {
    return (build / (name + Serialize(type, flavor))).generic_string();
}

Path Target::GetBuildPath(const Path& origin, const Path& new_root) {
    Path relative = std::filesystem::relative(origin, root);
    return new_root / relative;
}

void Target::AddDepend(std::shared_ptr<Target> target) {
    deps.push_back(target);
}

void Target::AddDepends(std::vector<std::shared_ptr<Target>>& targets) {
    deps.append_range(targets);
}

void Target::AddDepends(
    std::initializer_list<std::shared_ptr<Target>> targets) {
    deps.append_range(targets);
}

void Target::AddInclude(const Path& include) {
    flags.push_back("-I" + include.generic_string());
}

void Target::AddIncludes(const std::vector<Path>& includes) {
    for (auto& path : includes) {
        flags.push_back("-I" + path.generic_string());
    }
}

void Target::AddIncludes(std::initializer_list<Path> includes) {
    for (auto& path : includes) {
        flags.push_back("-I" + path.generic_string());
    }
}

void Target::AddSource(const Path& source) { units.push_back(source); }

void Target::AddSources(const std::vector<Path>& sources) {
    units.append_range(sources);
}

void Target::AddSources(std::initializer_list<Path> sources) {
    units.append_range(sources);
}

Target::Target(const std::string& name, TargetType type, const Path& work_root,
               const Path& build_dir, std::shared_ptr<ToolChain> tool_chain,
               const std::vector<Path>& sources,
               const std::vector<std::string>& flags,
               const std::vector<std::shared_ptr<Target>>& targets)
    : name(name),
      type(type),
      root(work_root),
      build(build_dir),
      tool_chain(tool_chain),
      units(sources),
      flags(flags),
      deps(targets) {
    if (build.empty()) build = work_root / "build";
}

namespace make {

Build exec_target(const std::string& name, const Path& work_root) {
    return std::make_shared<Target>(name, TargetType::exec, work_root,
                                    work_root / "build");
}

Build static_target(const std::string& name, const Path& work_root) {
    return std::make_shared<Target>(name, TargetType::arch, work_root,
                                    work_root / "build");
}

Build dynamic_target(const std::string& name, const Path& work_root) {
    return std::make_shared<Target>(name, TargetType::share, work_root,
                                    work_root / "build");
}
} // namespace make

std::string Serialize(TargetType type) {
    switch (type) {
        case TargetType::exec: return "exec";
        case TargetType::arch: return "arch";
        case TargetType::share: return "share";
    }
}
} // namespace csc
