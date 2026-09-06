#pragma once
#include <filesystem>
#include <memory>
#include <vector>

namespace csc {
namespace fs = std::filesystem;

class Unit {
public:
    Unit(const fs::path& Path);

private:
    fs::path path;
};

class UnitInfo {
public:
    std::vector<std::shared_ptr<fs::path>> deps;
    fs::path obj;
    std::shared_ptr<Unit> unit;
};
} // namespace csc
