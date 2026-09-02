#include <rsc/os.h>

#include <pwd.h>
#include <unistd.h>

namespace os {
fs::path get_home_dir() {
    if (const char* home = std::getenv("HOME")) { return home; }

    passwd pwd{};
    passwd* result = nullptr;

    long size = sysconf(_SC_GETPW_R_SIZE_MAX);
    if (size < 0) size = 16384;

    std::vector<char> buffer(static_cast<size_t>(size));

    if (getpwuid_r(getuid(), &pwd, buffer.data(), buffer.size(), &result) ==
            0 &&
        result != nullptr) {
        return pwd.pw_dir;
    }

    throw std::runtime_error("failed to get home directory");
}

bool valid_name(std::string_view name) {
    if (name.empty() || name == "." || name == "..") return false;

    if (name.find_first_of("/\0", 0, 2) != std::string_view::npos) return false;

    return true;
}
} // namespace os
