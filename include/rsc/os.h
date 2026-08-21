#pragma once
#include <filesystem>

namespace os {
namespace fs = std::filesystem;

// TODO: impl in linux
fs::path get_home_dir();
} // namespace os
