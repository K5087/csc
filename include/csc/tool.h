#pragma once
#include <filesystem>
#include <optional>
#include <vector>

namespace csc {
namespace fs = std::filesystem;
std::vector<fs::path> find_file(const std::vector<fs::path>& path,
                                const std::string& extension);

std::vector<fs::path> find_file(const std::vector<fs::path>& path,
                                const std::vector<std::string>& extension);
std::optional<std::string> read_file(const fs::path& path);
} // namespace csc
