#pragma once
#include <csc/depend.h>

#include <filesystem>

namespace csc {
namespace fs = std::filesystem;

class Unit {
public:
    Unit() = delete;
    Unit(const fs::path& Path, const fs::path& build_path);

public:
    fs::path path;
    fs::path obj;
    fs::path pcm;
    // depend file
    fs::path depend;
    // p1689(module depend)
    fs::path p1689;
    std::string path_string;
    std::string obj_path;
    std::string pcm_path;
    std::string depend_path;
    std::string p1689_path;

    bool is_module = false;
    dep::Info info;
    std::vector<std::pair<const std::string&, std::size_t>> require;
};

} // namespace csc
