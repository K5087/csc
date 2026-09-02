#pragma once
#include <csc/tool_chain.h>

#include <filesystem>
#include <source_location>
#include <string>
#include <vector>

namespace csc {
using Path = std::filesystem::path;
#define CURRENT_PATH \
    std::filesystem::absolute(std::source_location::current().file_name())

#define CURRENT_DIR CURRENT_PATH.parent_path()

enum class TargetType {
    exec,
    arch,
    share,
};

enum class TargetFlavor { GNU, MSVC };

std::string Serialize(TargetType type, TargetFlavor flavor);
std::string Serialize(TargetType type);

class Target {
public:
    Target() = delete;
    Target(const std::string& name, TargetType type, const Path& work_root,
           const Path& build = {},
           std::shared_ptr<ToolChain> tool_chain = get_default_toolchain(),
           const std::vector<Path>& sources = {},
           const std::vector<std::string>& flags = {},
           const std::vector<std::shared_ptr<Target>>& targets = {});

public:
    std::string GetTarget() const;
    Path GetBuildPath(const Path& origin, const Path& new_root);

public:
    Path root;  // 项目根目录
    Path build; // 项目build目录

    TargetType type;                         // 构建类型
    TargetFlavor flavor = TargetFlavor::GNU; // 构建风格
    std::shared_ptr<ToolChain> tool_chain;   // 构建使用的工具链

    std::string name;                    // 目标名称
    std::vector<Path> units = {};        // 目标源码
    std::vector<std::string> flags = {}; // 目标添加的flag

    std::vector<std::shared_ptr<Target>> deps = {}; // 依赖的其他目标
    // 链接系统库(怎么还有-luuid这种不指定依赖目标,靠库搜索的依赖)
    std::vector<std::string> searches = {};

    bool is_build = false;

    // convert to absolute path and add to units
    void AddSource(const Path& source);
    void AddSources(const std::vector<Path>& sources);
    void AddSources(std::initializer_list<Path> sources);

    // TODO:这里需要做一下依赖循环检测
    void AddDepend(std::shared_ptr<Target> target);
    void AddDepends(std::vector<std::shared_ptr<Target>>& targets);
    void AddDepends(std::initializer_list<std::shared_ptr<Target>> targets);

    // convert to absolute path and -I to flags
    void AddInclude(const Path& includes);
    void AddIncludes(const std::vector<Path>& includes);
    void AddIncludes(std::initializer_list<Path> includes);
};

using Build = std::shared_ptr<Target>;

std::string_view get_extension(TargetType type = TargetType::exec,
                               TargetFlavor flavor = TargetFlavor::GNU);

namespace make {
Build exec_target(const std::string& name, const Path& work_root = CURRENT_DIR);
Build static_target(const std::string& name,
                    const Path& work_root = CURRENT_DIR);
Build dynamic_target(const std::string& name,
                     const Path& work_root = CURRENT_DIR);

} // namespace make
} // namespace csc
