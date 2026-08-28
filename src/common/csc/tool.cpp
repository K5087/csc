#include <csc/tool.h>

#include <cassert>
#include <fstream>

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

std::optional<std::string> read_file(const fs::path& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) { return std::nullopt; }

    const auto end = file.tellg();
    if (end < 0) { return std::nullopt; }

    std::string buff(static_cast<std::size_t>(end), '\0');

    file.seekg(0, std::ios::beg);

    if (!buff.empty()) {
        file.read(buff.data(), static_cast<std::streamsize>(buff.size()));

        if (!file) { return std::nullopt; }
    }

    return buff;
}
} // namespace csc
