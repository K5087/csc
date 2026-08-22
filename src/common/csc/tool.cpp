#include <csc/tool.h>

#include <cassert>

namespace csc {

std::vector<fs::path> find_file(const std::vector<fs::path>& paths,
                                const std::string& extension) {
    std::vector<fs::path> result;

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

std::vector<fs::path> find_file(const std::vector<fs::path>& paths,
                                const std::vector<std::string>& extension) {
    std::vector<fs::path> result;

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
