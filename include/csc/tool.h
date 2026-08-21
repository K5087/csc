#pragma once
#include <filesystem>
#include <vector>

namespace csc {
using Path = std::filesystem::path;
std::vector<Path> find_file(const std::vector<Path>& path,
                            const std::string& extension);

std::vector<Path> find_file(const std::vector<Path>& path,
                            const std::vector<std::string>& extension);
} // namespace csc
