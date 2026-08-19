#pragma once
#include <csc/tool_chain.h>

#include <filesystem>
#include <string>
#include <vector>

namespace csc {
using Path = std::filesystem::path;

enum class BuildType {
    exe,
    lib,
    dll,
};

enum class TargetFlavor {
    GNU,
    MSVC
};

std::string Serialize(BuildType type, TargetFlavor flavor);

class Target {
public:
    std::string GetTarget() const;
    Path GetBuildPath(const Path& origin, const Path& new_root);

public:
    Path root;
    Path build = root / "csc_build";

    BuildType type = BuildType::exe;
    TargetFlavor flavor = TargetFlavor::GNU;
    std::shared_ptr<ToolChain> tool_chain;

    std::string name;
    std::vector<Path> units = {};
    std::vector<std::string> flags = {};

    std::vector<std::shared_ptr<Target>> deps = {};

    bool is_build = false;

    void AddDepend(std::shared_ptr<Target> target);
    void AddDepends(std::vector<std::shared_ptr<Target>>& targets);
    void AddDepends(std::initializer_list<std::shared_ptr<Target>> targets);
    void AddInclude(const Path& includes);
    void AddIncludes(const std::vector<Path>& includes);
    void AddIncludes(std::initializer_list<Path> includes);
};

} // namespace csc
