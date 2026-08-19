#include <csc/target.h>

namespace csc {

std::string Serialize(BuildType type, TargetFlavor flavor) {
    switch (type) {
        case BuildType::exe: return ".exe";
        case BuildType::lib:
            switch (flavor) {
                case TargetFlavor::GNU: return ".a";
                case TargetFlavor::MSVC: return ".lib";
            };
        case BuildType::dll: return ".dll";
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

void Target::AddDepends(std::initializer_list<std::shared_ptr<Target>> targets) {
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
} // namespace csc
