#pragma once
#include <csc/tool_chain.h>

#include <filesystem>
#include <source_location>
#include <string>
#include <vector>

namespace csc {
using Path = std::filesystem::path;
#define CURRENT_PATH \
    std::filesystem::path(std::source_location::current().file_name())
#define CURRENT_DIR CURRENT_PATH.parent_path()

enum class BuildType {
    exe,
    lib,
    dll,
};

enum class TargetFlavor { GNU, MSVC };

std::string Serialize(BuildType type, TargetFlavor flavor);

class Target {
public:
    Target() = delete;
    Target(const std::string& name, BuildType type, const Path& work_root,
           const Path& build = {},
           std::shared_ptr<ToolChain> tool_chain = get_default_toolchain(),
           const std::vector<Path>& sources = {},
           const std::vector<std::string>& flags = {},
           const std::vector<std::shared_ptr<Target>>& targets = {});

public:
    std::string GetTarget() const;
    Path GetBuildPath(const Path& origin, const Path& new_root);

public:
    Path root;
    Path build;

    BuildType type;
    TargetFlavor flavor = TargetFlavor::GNU;
    std::shared_ptr<ToolChain> tool_chain;

    std::string name;
    std::vector<Path> units = {};
    std::vector<std::string> flags = {};

    std::vector<std::shared_ptr<Target>> deps = {};
    std::vector<std::string> searches = {};

    bool is_build = false;

    void AddSource(const Path& source);
    void AddSources(const std::vector<Path>& sources);
    void AddSources(std::initializer_list<Path> sources);

    void AddDepend(std::shared_ptr<Target> target);
    void AddDepends(std::vector<std::shared_ptr<Target>>& targets);
    void AddDepends(std::initializer_list<std::shared_ptr<Target>> targets);

    void AddInclude(const Path& includes);
    void AddIncludes(const std::vector<Path>& includes);
    void AddIncludes(std::initializer_list<Path> includes);
};

using Build = std::shared_ptr<Target>;

namespace make {
Build exec_target(const std::string& name, const Path& work_root = CURRENT_DIR);
Build static_target(const std::string& name,
                    const Path& work_root = CURRENT_DIR);
Build dynamic_target(const std::string& name,
                     const Path& work_root = CURRENT_DIR);

} // namespace make
} // namespace csc
