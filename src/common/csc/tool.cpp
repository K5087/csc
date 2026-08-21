#include <csc/tool.h>

#include <cassert>

namespace csc {
namespace fs = std::filesystem;

std::vector<Path> find_file(const std::vector<Path>& paths,
                            const std::string& extension) {
    std::vector<Path> result;

    for (const auto& root : paths) {
        if (!fs::exists(root)) { continue; }
        for (const auto& entry :
             std::filesystem::recursive_directory_iterator(root)) {
            if (entry.path().extension() == extension) {
                // result.push_back(absolute(entry.path()));
                result.push_back((entry.path()));
            }
        }
    }

    return result;
}

std::vector<Path> find_file(const std::vector<Path>& paths,
                            const std::vector<std::string>& extension) {
    std::vector<Path> result;

    for (const auto& root : paths) {
        if (!fs::exists(root)) { continue; }
        for (const auto& entry :
             std::filesystem::recursive_directory_iterator(root)) {
            if (std::find(extension.begin(), extension.end(),
                          entry.path().extension()) != extension.end()) {
                // result.push_back(absolute(entry.path()));
                result.push_back((entry.path()));
            }
        }
    }

    return result;
}

} // namespace csc
