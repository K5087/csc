#pragma once
#include <filesystem>
#include <vector>

namespace csc {
namespace fs = std::filesystem;
std::vector<fs::path> find_file(const std::vector<fs::path>& path,
                                const std::string& extension);

std::vector<fs::path> find_file(const std::vector<fs::path>& path,
                                const std::vector<std::string>& extension);

} // namespace csc
