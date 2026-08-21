#include <rsc/os.h>

#include <shlobj.h>
#include <windows.h>

namespace os {
fs::path get_home_dir() {
    PWSTR path = nullptr;

    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Profile, 0, nullptr, &path))) {
        std::filesystem::path result(path);
        CoTaskMemFree(path);
        return result;
    }

    return {};
}
} // namespace os
