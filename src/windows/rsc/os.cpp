#include <rsc/os.h>

#include <shlobj.h>
#include <windows.h>

#include <algorithm>

namespace os {
fs::path get_home_dir() {
    PWSTR path = nullptr;

    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Profile, 0, nullptr, &path))) {
        std::filesystem::path result(path);
        CoTaskMemFree(path);
        return result;
    }

    throw std::runtime_error("failed to get home directory");
}

bool is_reserved_name(std::string_view name) {
    auto base = name.substr(0, name.find('.'));

    std::string upper(base);
    std::ranges::transform(upper, upper.begin(), [](unsigned char c) {
        return static_cast<char>(std::toupper(c));
    });

    if (upper == "CON" || upper == "PRN" || upper == "AUX" || upper == "NUL")
        return true;

    if (upper.size() == 4 &&
        (upper.starts_with("COM") || upper.starts_with("LPT")) &&
        upper[3] >= '1' && upper[3] <= '9')
        return true;

    return false;
}

bool valid_name(std::string_view name) {
    if (name.empty() || name == "." || name == "..") return false;

    if (name.ends_with('.') || name.ends_with(' ')) return false;

    if (name.find_first_of("<>:\"/\\|?*") != std::string_view::npos)
        return false;

    // for (unsigned char c : name) {
    //     if (c < 0x20) return false;
    // }

    // if (is_reserved_name(name)) { return false; }

    return true;
}
} // namespace os
