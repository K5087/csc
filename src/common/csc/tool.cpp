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

bool write_file(const fs::path& path, const std::string& data) {
    std::ofstream file(path, std::ios::binary);
    if (!file) { return false; }
    file << data;
    if (!file) { return false; }
    return true;
}

std::vector<fs::path> parse_dep(const std::string& data, const fs::path& root) {
    std::vector<fs::path> paths;

    auto iter = data.begin();
    std::string str;
    auto push = [&]() {
        if (!str.empty()) {
            paths.push_back(root / str);
            str.clear();
        }
    };
    while (iter != data.end()) {
        switch (*iter) {
            case ':':
                if (std::next(iter) != data.end() && *std::next(iter) == ' ') {
                    str.clear();
                    iter++;
                    break;
                }
                str += *iter;
                break;
            case ' ': push(); break;
            case '\r': push(); break;
            case '\n': push(); break;
            case '\\': {
                auto next = std::next(iter);
                if (next == data.end()) {
                    str += *iter;
                    break;
                }

                switch (*next) {
                    case '\n': ++iter; break;
                    case '\r': {
                        auto next2 = std::next(next);
                        if (next2 != data.end() && *next2 == '\n')
                            iter += 2;
                        else
                            str += *iter;
                        break;
                    }
                    default: str += *iter; break;
                }
                break;
            }
            default: str += *iter;
        }
        iter++;
    }
    push();
    return paths;
}
} // namespace csc
