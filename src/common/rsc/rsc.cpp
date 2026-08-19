#include <rsc/rsc.h>

namespace rsc {
using std::filesystem::absolute;

void init() {
    if (!std::filesystem::exists(script)) {
        std::filesystem::create_directory(script);
    }
}

std::vector<Path> find_file(const std::vector<Path>& paths, const std::string& extension) {
    std::vector<Path> result;

    for (const auto& root : paths) {
        if (!std::filesystem::exists(root)) {
            continue;
        }
        for (const auto& entry :
             std::filesystem::recursive_directory_iterator(root)) {
            if (entry.path().extension() == extension) {
                result.push_back(absolute(entry.path()));
            }
        }
    }

    return result;
}
} // namespace rsc
