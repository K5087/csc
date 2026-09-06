#pragma once
#include <filesystem>

namespace os {
namespace fs = std::filesystem;

fs::path get_home_dir();
bool valid_name(std::string_view name);
} // namespace os
